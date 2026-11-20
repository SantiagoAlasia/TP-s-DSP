% monitor.m - Receptor no bloqueante para TP4 (MCXN947 en Linux)

function monitor()
clear; clc; close all;
pkg load instrument-control

%% ----------------------------- Configuracion -----------------------------
PUERTO     = "/dev/ttyACM2";
BAUDRATE   = 115200;
MOSTRAR_DW = true;
DEBUG_RX   = false;   % Cambia a true si quieres ver en crudo todo lo que llega
%% -------------------------------------------------------------------------

NT   = 30;
BLOQ = 100;

% Coeficientes de la planta (los mismos de fir_planta.h, Q15 -> float)
PLANTA_Q15 = [16384, 12292, 5755, -883, -5782, -7992, -7514, -5085, -1811, 1220, 3234, 3907, 3358, 2013, 420, -929, -1719, -1860, -1459, -749, 6, 589, 879, 863, 614, 253, -96, -340, -436, -390];
PLANTA = PLANTA_Q15 / 32768;

nombres = {'OFF', 'ID random', 'ID constante', 'Cancelacion'};

s = serialport(PUERTO, BAUDRATE);
set(s, "timeout", 1);
flush(s);
limpiar = onCleanup(@() clear("s"));

fig = figure("name", "TP4 DSP - MCXN947");
E = struct('seqs', [], 'mse', {{}}, 'nd', {{}}, 'etiq', {{}});
ult_dw = zeros(1, NT);
ult_seq = 0;
t_dib = tic;

buffer = "";

printf("Escuchando en %s a %d baudios... (Presiona el boton de modo en la placa)\n", PUERTO, BAUDRATE);

while ishandle(fig)

    % ---- 1. Lectura NO bloqueante (igual que fft_monitor.m) ----
    n = s.NumBytesAvailable;
    if n > 0
        buffer = [buffer, char(read(s, n))];
    else
        pause(0.01);   % Evita que Octave se trabe cuando no llegan datos
    end

    % ---- 2. Procesar lineas completas terminadas en \n ----
    pos = strfind(buffer, "\n");
    while !isempty(pos)
        l      = strtrim(buffer(1:pos(1)-1));
        buffer = buffer(pos(1)+1:end);
        pos    = strfind(buffer, "\n");

        if isempty(l), continue; end

        if DEBUG_RX
            printf("rx: [%s]\n", l);
        end

        % ---- 3. Decodificacion del protocolo TP4 ----
        if l(1) == '#'
            printf("%s\n", l);

        elseif numel(l) > 2 && l(1) == 'C' && l(2) == ','
            v = str2double(strsplit(l(3:end), ","));
            if numel(v) == 4 && !any(isnan(v))
                [E, i] = buscar(E, v(1));
                idx_modo = min(max(v(2)+1, 1), numel(nombres));
                E.etiq{i} = sprintf('%s, mu=%.3g', nombres{idx_modo}, v(4)/32768);
                printf('--- seq %d: %s | mu_idx=%d | mu=%.3g ---\n', v(1), nombres{idx_modo}, v(3), v(4)/32768);
            else
                printf('[linea C invalida] %s\n', l);
            end

        elseif numel(l) > 2 && l(1) == 'D' && l(2) == ','
            v = str2double(strsplit(l(3:end), ","));
            if numel(v) == (2 + NT) && !any(isnan(v))
                [E, i] = buscar(E, v(1));
                mse = max(v(2) / 2^30, 1e-10);
                dw  = v(3:end) / 32768;
                nd  = max(sqrt(sum(dw.^2)), 1e-10);

                E.mse{i}(end+1) = mse;
                E.nd{i}(end+1)  = nd;
                ult_dw  = dw;
                ult_seq = v(1);

                printf('seq %d  bloque %3d  MSE = %.3e  ||dw|| = %.4f\n', v(1), numel(E.mse{i}), mse, nd);
                if MOSTRAR_DW
                    printf('   dw = %s\n', mat2str(dw, 4));
                end

                if toc(t_dib) > 0.15 && ishandle(fig)
                    dibujar(E, ult_dw, ult_seq, BLOQ, PLANTA);
                    t_dib = tic;
                end
            else
                printf('[linea D incompleta: %d valores] %s\n', numel(v), l(1:min(end,50)));
            end
        else
            printf('[?] %s\n', l);
        end
    end
end

function [E, i] = buscar(E, seq)
    i = find(E.seqs == seq, 1);
    if isempty(i)
        E.seqs(end+1) = seq;
        i = numel(E.seqs);
        E.mse{i} = [];  E.nd{i} = [];  E.etiq{i} = sprintf('seq %d', seq);
    end
end

function dibujar(E, ult_dw, ult_seq, BLOQ, PLANTA)
    col = lines(max(numel(E.seqs), 1));

    subplot(2, 2, 1); cla; hold on;
    for i = 1:numel(E.seqs)
        if ~isempty(E.mse{i})
            semilogy((1:numel(E.mse{i})) * BLOQ, E.mse{i}, 'Color', col(i,:), 'LineWidth', 1.5);
        end
    end
    grid on; xlabel('muestras'); ylabel('MSE'); title('MSE por bloque');
    if ~isempty(E.etiq), legend(E.etiq, 'Location', 'northeast'); end

    %subplot(2, 2, 2); cla; hold on;
    %for i = 1:numel(E.seqs)
    %    if ~isempty(E.nd{i})
    %        semilogy((1:numel(E.nd{i})) * BLOQ, E.nd{i}, 'Color', col(i,:), 'LineWidth', 1.5);
    %    end
    %end
    %grid on; xlabel('muestras'); ylabel('||dw||'); title('Distancia entre coeficientes');

    subplot(2, 2, 2); cla;
    stem(0:numel(ult_dw)-1, ult_dw, 'LineWidth', 1.2);
    grid on; xlabel('k'); ylabel('dw[k]'); title(sprintf('dw[k] = h[k] - w[k] (ultimo bloque, seq %d)', ult_seq));

    % Coeficientes de la planta (hardcodeados)
    subplot(2, 2, 3); cla;
    stem(0:numel(PLANTA)-1, PLANTA, 'LineWidth', 1.2);
    grid on; xlabel('k'); ylabel('h[k]'); title('Coeficientes de la planta (fijos)');

    % Planta vs. estimacion del adaptativo: w = h - dw
    w_est = PLANTA - ult_dw;
    subplot(2, 2, 4); cla; hold on;
    stem(0:numel(PLANTA)-1, PLANTA, 'b', 'LineWidth', 1.2);
    stem(0:numel(w_est)-1, w_est, 'r', 'LineWidth', 1.2);
    grid on; xlabel('k'); ylabel('coef'); title('Planta h[k] vs. adaptativo w[k]');
    legend('planta h', 'adaptativo w', 'Location', 'northeast');

    drawnow;
end
end
