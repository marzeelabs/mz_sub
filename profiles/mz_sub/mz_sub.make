; =============================================================
;                     MZ Sub Makefile
; =============================================================

api = 2
core = 7.x

; Download the base install profile and recursively build all its dependencies:
projects[mz][type] = "profile"
projects[mz][download][type] = "git"
projects[mz][download][url] = "git@github.com:marzeelabs/mz.git"
; The branch or version to run from; you can change this to run a different version
; of the base profile
projects[mz][download][branch] = "master"


; Projects
; --------

projects[fb_likebox][version] = 1.8
projects[fb_likebox][subdir] = "contrib"
