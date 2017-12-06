% A simple example of the LMS algorithm written in Matlab
% The C function 'my_lms' should be close to what's done in the file 'my_lms.m'

% In this file, call the LMS algorithm with a bunch of "recorded" data and draw some plots

clear variables
close all
format short eng
clc

%Simulation set-up
fs = 16e3;		%System sample-rate
T = 3;			%Total simulation time

%Test with broad-band noise
lms_state = rand([1,T*fs]);	%Disturbance data

%Simulated channel
h = [1,-2,3];

%Generate x simply as the convolution of h and the disturbance
x = conv(lms_state,h);
x = x(length(h):end-length(h)+1);

%Set up LMS algorithm parameters
mu = 1e-3;
lms_coeffs = zeros(size(h));	%Initialize the h_hat to all zeros (arbitrary choice)
block_size = length(lms_state) - length(lms_coeffs) + 1;	%For convenience, set the "block size" to be all the data we need to process

%Perform all LMS filtering
[xhat, e, ~, lms_coeffs_hist] = my_lms(lms_state, lms_coeffs, x, block_size, mu);

%LMS coefficients end up being stored in reverse order, flip to generate
%h_hat
h_hat_hist = flipud(lms_coeffs_hist.');

fprintf(['   Actual channel impulse response h = ' num2str(h,'%10f') '\n']);
fprintf(['Estimated channel impulse response h = ' num2str(h_hat_hist(:,end).', '%10f') '\n']);

%Generate some plots

%Create x-axis vector
n = 1:block_size;
t = n*1/fs;

figure(1);
plot(t,e);
title('Filter error $e[n]$', 'Interpreter', 'latex');
xlabel('Time [s]');
ylabel('Error [-]');

figure(2);
yyaxis left
fig_h = plot(t, h_hat_hist);
ax = axis();
axis(ax .* [1 1 1.2 1.2]); %Increase y extent
title('Estimated filter/actual channel coefficient history');
xlabel('Time [s]');
ylabel('Estimated h[n]');
h_n = num2str(cumsum(ones(length(fig_h),1)));


yyaxis right
plot(t, repmat(h(:), 1, length(t)));
axis(ax .* [1 1 1.2 1.2]); %Increase y extent

labels_hat = arrayfun(@(x) ['$\hat{h}[', x, ']$'], h_n, 'un', false);
labels_act = arrayfun(@(x) ['$h[', x, ']$'], h_n, 'un', false);
legend([labels_hat; labels_act], 'Interpreter', 'latex');
ylabel('Actual h[n]');