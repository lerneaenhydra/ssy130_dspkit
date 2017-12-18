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

sig = x/max(x);	%Normalize amplitude

plot(abs(fft(sig)));
title('FT of resultant signal');
xlabel('FFT index');
ylabel('Magnitude');

sound(repmat(sig(:), 50, 1), 16000);

res = fir_coeffs2c('dist', sig);
disp(res);