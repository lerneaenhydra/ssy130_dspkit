%Generate colored gaussian noise
clc
clear all
close all
format short eng

%Generate wide-band disturbance with flat frequency spectrum in range [eps,
%fs/4] (where eps is the second frequency bin) of length 256
X = zeros(256,1);
X(2:end/4) = 1;
x = real(ifft(X));

fs = 16e3;

f = linspace(0,fs,length(x));

sig = x/max(abs(x));	%Normalize amplitude

plot(f, abs(fft(sig)));
title('Fourier transform of broadband disturbance');
ylabel('Magnitude [-]');
xlabel('Frequency [Hz]');

print('noise_ft.eps','-depsc');


sound(repmat(sig(:), 50, 1), 16000);

res = fir_coeffs2c('dist', sig);
disp(res);