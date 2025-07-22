pkg signal load
filtord = 25 % Dlugość filtru
Fs = 50; % Częstotliwość próbkowania 50Hz
FNq = Fs/2; % Częstotliwość Nyquista
F1c = 2; % Pasmo przepustowe 0 - 2Hz
F2c = 5; % Pasmo zaporowe 5 Hz ->
F3c = 25; % Pasmo zaporowe <- 25 Hz
f=[0,F1c/FNq,F2c/FNq,F3c/FNq]
m = [ 1 , 1 , 0, 0 ]
freqz ( remez(filtord,f,m) );
[h, w] = freqz ( remez(filtord,f,m) );
subplot(2,1,1);
plot (f, m, '', w/pi, abs (h), '');
xlabel('Znormalizowana czestotliwosc')
ylabel('wzmocnienie')
grid on
subplot(2,1,2);
plot(f,20*log10(m+1e-5),'', w/pi,20*log10(abs(h)),'');
xlabel('Znormalizowana czestotliwosc')
ylabel('wzmocnienie (dB)')
grid on
remez(filtord,f,m)
floor ( remez(filtord,f,m) * 32767 )
