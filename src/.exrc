if &cp | set nocp | endif
let s:cpo_save=&cpo
set cpo&vim
map <NL> j
map  k
map Q gq
nmap gx <Plug>NetrwBrowseX
nnoremap <silent> <Plug>NetrwBrowseX :call netrw#NetrwBrowseX(expand("<cWORD>"),0)
let &cpo=s:cpo_save
unlet s:cpo_save
set autoindent
set autowriteall
set backspace=2
set fileencodings=ucs-bom,utf-8,default,latin1
set helplang=en
set incsearch
set nojoinspaces
set makeprg=make\ -j4
set printoptions=paper:letter
set ruler
set runtimepath=~/.vim,/var/lib/vim/addons,/usr/share/vim/vimfiles,/usr/share/vim/vim74,/usr/share/vim/vimfiles/after,/var/lib/vim/addons/after,~/.vim/after
set showcmd
set showmatch
set suffixes=.bak,~,.swp,.o,.info,.aux,.log,.dvi,.bbl,.blg,.brf,.cb,.ind,.idx,.ilg,.inx,.out,.toc
set viminfo='20,\"50
set visualbell
set nowritebackup
" vim: set ft=vim :
