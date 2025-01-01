let g:ale_c_cc_options="-std=c11 -Wall -I/usr/local/include -I/opt/homebrew/include -I/opt/homebrew/opt/curl/include -I /opt/homebrew/opt/curl/include/"
let g:ale_linters_explicit=1
let g:ale_linters = {'c': ['clang'],}
set complete=.,w,b,u,t
