%Generate colored gaussian noise
clc
clear all
close all
format short eng

wgn = randn([2048,1]);

%Band-limit to range [0, 0.1]*fs/2
b = firpm(51, [0 0.05 0.15 1], [1 1 0 0]);

%freqz(b);

sig = cconv(wgn, b, length(wgn));
sig = sig * 0.1;	%Scale amplitude

plot(abs(fft(sig)));
title('FT of resultant signal');
xlabel('FFT index');
ylabel('Magnitude');

sound(repmat(sig(:), 30, 1), 44100);

res = fir_coeffs2c('dist', sig);
disp(res);