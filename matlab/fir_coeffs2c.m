function [c_str] = fir_coeffs2c(str_name, coeffs)
%FIR_COEFFS2C Converts a vector of FIR filter coefficients and a name to
%an equivalent C representation.
%   str = FIR_COEFFS2C('foo', [1, 0, -1]) returns a string
%   'const float foo[3] = {1, 0, -1};'

c_str = sprintf('#define %s_TAPS (%d)', upper(str_name), length(coeffs));
c_str = sprintf('%s\n float %s[%s_TAPS] = {', c_str, str_name, upper(str_name));

for i=1:length(coeffs)
    c_str = sprintf('%s%e', c_str, coeffs(i));
    if(i == length(coeffs))
        c_str = sprintf('%s};', c_str);
    else
        c_str = sprintf('%s, ', c_str);
    end
end
end

