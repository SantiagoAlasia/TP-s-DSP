% recibir_serial.m - Recibe por el puerto serie los mensajes del TP4, los decodifica,
% muestra los valores en la consola y los grafica en tiempo real.
%
% Uso:   recibir_serial()                              % /dev/ttyACM0 a 115200
%        recibir_serial('/dev/ttyACM0', 115200)        % Linux
%        recibir_serial('COM5', 115200)                % Windows
%        recibir_serial('/dev/ttyACM0', 115200, true)  % ademas imprime los 30 dw de cada bloque
%
% Se detiene al cerrar la figura (o con Ctrl+C).
% Requiere el paquete instrument-control de Octave.
%
% Mensajes (ver uart_proto.h), una linea cada uno, terminada en \r\n:
%   C,seq,modo,mu_idx,mu_q15            configuracion
%   D,seq,mse_q30,dw0,...,dw29          datos (uno por bloque de 100 muestras)
%   # texto                             comentario

function Visualizar()
  pkg load instrument-control

  puerto = '/dev/ttyACM0';
  baud = 115200;
  mostrar_dw = false;

  NT = 30;                 % coeficientes
  BLOQ = 100;              % muestras por bloque
  nombres = {'OFF', 'ID random', 'ID constante', 'Cancelacion'};

  % ---------- abrir el puerto ----------
  s = serialport(puerto, baud);
  configureTerminator(s, 'cr/lf');
  s.Timeout = 1;           % readline devuelve vacio si no llega nada en 1 s
  flush(s);                % descarta lo que haya quedado en el buffer

  fig = figure('Name', 'TP4 DSP');
  E = struct('seqs', [], 'mse', {{}}, 'nd', {{}}, 'etiq', {{}});   % datos por experimento
  ult_dw = zeros(1, NT);  ult_seq = 0;
  t_dib = tic;
  printf('Escuchando %s a %d baudios... (cerrar la figura para terminar)\n', puerto, baud);

  unwind_protect
    while ishandle(fig)
      l = strtrim(readline(s));
      if isempty(l), continue; end

      if l(1) == '#'                                        % comentario
        printf('%s\n', l);

      elseif numel(l) > 2 && l(1) == 'C' && l(2) == ','     % configuracion
        v = sscanf(l(3:end), '%d,')';
        if numel(v) == 4
          [E, i] = buscar(E, v(1));
          E.etiq{i} = sprintf('%s, mu=%.3g', nombres{v(2)+1}, v(4)/32768);
          printf('--- seq %d: %s | mu_idx=%d | mu=%.3g ---\n', v(1), nombres{v(2)+1}, v(3), v(4)/32768);
        else
          printf('[linea C invalida] %s\n', l);
        end

      elseif numel(l) > 2 && l(1) == 'D' && l(2) == ','     % datos
        v = sscanf(l(3:end), '%d,')';
        if numel(v) == 2 + NT
          [E, i] = buscar(E, v(1));
          mse = v(2) / 2^30;                                % MSE real
          dw  = v(3:end) / 32768;                           % diferencia de coeficientes, real
          nd  = sqrt(sum(dw.^2));                           % ||dw||
          E.mse{i}(end+1) = max(mse, 1e-10);                % piso para la escala log
          E.nd{i}(end+1)  = max(nd,  1e-10);
          ult_dw = dw;  ult_seq = v(1);
          printf('seq %d  bloque %3d  MSE = %.3e  ||dw|| = %.4f\n', v(1), numel(E.mse{i}), mse, nd);
          if mostrar_dw
            printf('   dw = %s\n', mat2str(dw, 4));
          end
          if toc(t_dib) > 0.15
            dibujar(E, ult_dw, ult_seq, BLOQ);
            t_dib = tic;
          end
        else
          printf('[linea D incompleta: %d valores] %s\n', numel(v), l(1:min(end,40)));
        end

      else
        printf('[?] %s\n', l);                              % basura de arranque u otra cosa
      end
    end
  unwind_protect_cleanup
    clear s;                                                % cierra el puerto
    printf('Puerto cerrado.\n');
  end_unwind_protect
end

% Devuelve el indice del experimento seq (lo crea si no existe).
function [E, i] = buscar(E, seq)
  i = find(E.seqs == seq, 1);
  if isempty(i)
    E.seqs(end+1) = seq;
    i = numel(E.seqs);
    E.mse{i} = [];  E.nd{i} = [];  E.etiq{i} = sprintf('seq %d', seq);
  end
end

function dibujar(E, ult_dw, ult_seq, BLOQ)
  col = lines(max(numel(E.seqs), 1));

  subplot(2, 2, 1); cla; hold on;
  for i = 1:numel(E.seqs)
    if ~isempty(E.mse{i}), semilogy((1:numel(E.mse{i})) * BLOQ, E.mse{i}, 'Color', col(i,:), 'LineWidth', 1.5); end
  end
  grid on; xlabel('muestras'); ylabel('MSE'); title('MSE por bloque');
  legend(E.etiq, 'Location', 'northeast');

  subplot(2, 2, 2); cla; hold on;
  for i = 1:numel(E.seqs)
    if ~isempty(E.nd{i}), semilogy((1:numel(E.nd{i})) * BLOQ, E.nd{i}, 'Color', col(i,:), 'LineWidth', 1.5); end
  end
  grid on; xlabel('muestras'); ylabel('||dw||'); title('Distancia entre coeficientes (planta - adaptativo)');

  subplot(2, 1, 2); cla;
  stem(0:numel(ult_dw)-1, ult_dw, 'LineWidth', 1.2);
  grid on; xlabel('k'); ylabel('dw[k]'); title(sprintf('dw[k] del ultimo bloque (seq %d)', ult_seq));

  drawnow;
end
