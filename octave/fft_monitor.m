% fft_monitor_v2.m - Receptor y graficador del espectro FFT (Laboratorio 3, MCXN947)
%
% Cambios respecto a fft_monitor.m:
%   - Parser robusto: corta por "\n" y limpia con strtrim (tolera \r\n y \r\r\n).
%   - Frecuencia del pico con interpolacion parabolica (sobre magnitud lineal).
%
% Trama esperada (una linea por ventana):
%   FFT,<fs_Hz>,<N>,<mag0>,<mag1>,...,<mag(N/2-1)>\r\n
%
% Requisitos: pkg install -forge instrument-control

clear; clc; close all;
pkg load instrument-control

%% ----------------------------- Configuracion -----------------------------
PUERTO    = "/dev/ttyACM0";   % Windows: "COM3" | Linux: "/dev/ttyACM0"
BAUDRATE  = 115200;           % Debe coincidir con la consola de la placa
USAR_DB   = false;            % true = eje Y en dB, false = valores crudos
QUITAR_DC = true;             % true = ignora el bin 0 al graficar y al buscar el pico

INTERPOLAR = true;            % true = interpolacion parabolica del pico
DEBUG_RX  = false;            % true = imprime cada linea recibida (diagnostico)
%% -------------------------------------------------------------------------

s = serialport(PUERTO, BAUDRATE);
set(s, "timeout", 1);
flush(s);
limpiar = onCleanup(@() clear("s"));   % cierra el puerto al salir

fig = figure("name", "Espectro FFT - MCXN947");
buffer = "";
primera = true;      % la primera linea puede llegar cortada: se descarta
nventanas = 0;
nerrores  = 0;

printf("Escuchando en %s a %d baudios...\n", PUERTO, BAUDRATE);

while ishandle(fig)

    % ---- Leer lo que haya disponible y acumular en el buffer ----
    n = s.NumBytesAvailable;
    if n > 0
        buffer = [buffer, char(read(s, n))];
    else
        pause(0.01);
    end

    % ---- Procesar todas las lineas completas (terminan en \n) ----
    pos = strfind(buffer, "\n");
    while !isempty(pos)
        linea  = strtrim(buffer(1:pos(1)-1));   % strtrim saca \r, espacios, etc.
        buffer = buffer(pos(1)+1:end);
        pos    = strfind(buffer, "\n");

        if DEBUG_RX
            printf("rx: [%s]\n", linea(1:min(60, end)));
        end

        if primera
            primera = false;
            continue;
        end

        campos = strsplit(linea, ",");
        if numel(campos) < 4 || !strcmp(campos{1}, "FFT")
            continue;          % linea de debug u otro texto
        end

        fs  = str2double(campos{2});
        N   = str2double(campos{3});
        mag = str2double(campos(4:end));

        % Validacion: deben llegar exactamente N/2 bins
        if isnan(fs) || isnan(N) || numel(mag) != N/2 || any(isnan(mag))
            nerrores++;
            % printf("Trama invalida descartada (%d bins, esperados %d)\n", numel(mag), N/2);
            continue;
        end

        nventanas++;
        mag = mag(:)';          % fila

        % ---- Eje de frecuencias: f(k) = k*fs/N ----
        k  = 0:(N/2 - 1);
        f  = k * fs / N;
        df = fs / N;

        % ---- Pico (sin bin 0 si corresponde), siempre sobre magnitud lineal ----
        k0 = 1 + QUITAR_DC;               % indice MATLAB inicial (1 = bin 0)
        [pico, idx] = max(mag(k0:end));
        idx = idx + k0 - 1;               % indice MATLAB del maximo (bin = idx-1)

        delta  = 0;
        y_pico = pico;
        if INTERPOLAR && idx > k0 && idx < numel(mag)
            a = mag(idx-1);  b = mag(idx);  c = mag(idx+1);
            den = a - 2*b + c;
            if den != 0
                delta = 0.5 * (a - c) / den;        % entre -0.5 y +0.5
                if abs(delta) > 0.5, delta = 0; end % proteccion
                y_pico = b - 0.25 * (a - c) * delta;
            end
        end
        f_pico = (idx - 1 + delta) * fs / N;

        % ---- Escala para graficar ----
        if USAR_DB
            y     = 20*log10(max(mag, 1));          % evita log(0)
            y_mk  = 20*log10(max(y_pico, 1));
        else
            y     = mag;
            y_mk  = y_pico;
        end

        % ---- Grafico ----
        if !ishandle(fig), break; end
        figure(fig);
        plot(f(k0:end), y(k0:end), "b", f_pico, y_mk, "ro");
        grid on;
        xlabel("Frecuencia [Hz]");
        if USAR_DB, ylabel("Magnitud [dB]"); else, ylabel("Magnitud (Q15 crudo)"); end
        title(sprintf("fs = %d Hz | N = %d | res = %.2f Hz | pico = %.1f Hz | rx = %d", fs, N, df, f_pico, nventanas));
        xlim([f(k0), fs/2]);
        drawnow;
    end
end
