
#ifndef ONVIFDISPLAY_EXPORT_H
#define ONVIFDISPLAY_EXPORT_H

#ifdef ONVIFDISPLAY_STATIC_DEFINE
#  define ONVIFDISPLAY_EXPORT
#  define ONVIFDISPLAY_NO_EXPORT
#else
#  ifndef ONVIFDISPLAY_EXPORT
#    ifdef onvifdisplay_EXPORTS
        /* We are building this library */
#      define ONVIFDISPLAY_EXPORT __declspec(dllexport)
#    else
        /* We are using this library */
#      define ONVIFDISPLAY_EXPORT __declspec(dllimport)
#    endif
#  endif

#  ifndef ONVIFDISPLAY_NO_EXPORT
#    define ONVIFDISPLAY_NO_EXPORT 
#  endif
#endif

#ifndef ONVIFDISPLAY_DEPRECATED
#  define ONVIFDISPLAY_DEPRECATED __declspec(deprecated)
#endif

#ifndef ONVIFDISPLAY_DEPRECATED_EXPORT
#  define ONVIFDISPLAY_DEPRECATED_EXPORT ONVIFDISPLAY_EXPORT ONVIFDISPLAY_DEPRECATED
#endif

#ifndef ONVIFDISPLAY_DEPRECATED_NO_EXPORT
#  define ONVIFDISPLAY_DEPRECATED_NO_EXPORT ONVIFDISPLAY_NO_EXPORT ONVIFDISPLAY_DEPRECATED
#endif

/* NOLINTNEXTLINE(readability-avoid-unconditional-preprocessor-if) */
#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef ONVIFDISPLAY_NO_DEPRECATED
#    define ONVIFDISPLAY_NO_DEPRECATED
#  endif
#endif

#endif /* ONVIFDISPLAY_EXPORT_H */
