
#ifndef ONVIFREPLAY_EXPORT_H
#define ONVIFREPLAY_EXPORT_H

#ifdef ONVIFREPLAY_STATIC_DEFINE
#  define ONVIFREPLAY_EXPORT
#  define ONVIFREPLAY_NO_EXPORT
#else
#  ifndef ONVIFREPLAY_EXPORT
#    ifdef onvifreplay_EXPORTS
        /* We are building this library */
#      define ONVIFREPLAY_EXPORT __declspec(dllexport)
#    else
        /* We are using this library */
#      define ONVIFREPLAY_EXPORT __declspec(dllimport)
#    endif
#  endif

#  ifndef ONVIFREPLAY_NO_EXPORT
#    define ONVIFREPLAY_NO_EXPORT 
#  endif
#endif

#ifndef ONVIFREPLAY_DEPRECATED
#  define ONVIFREPLAY_DEPRECATED __declspec(deprecated)
#endif

#ifndef ONVIFREPLAY_DEPRECATED_EXPORT
#  define ONVIFREPLAY_DEPRECATED_EXPORT ONVIFREPLAY_EXPORT ONVIFREPLAY_DEPRECATED
#endif

#ifndef ONVIFREPLAY_DEPRECATED_NO_EXPORT
#  define ONVIFREPLAY_DEPRECATED_NO_EXPORT ONVIFREPLAY_NO_EXPORT ONVIFREPLAY_DEPRECATED
#endif

/* NOLINTNEXTLINE(readability-avoid-unconditional-preprocessor-if) */
#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef ONVIFREPLAY_NO_DEPRECATED
#    define ONVIFREPLAY_NO_DEPRECATED
#  endif
#endif

#endif /* ONVIFREPLAY_EXPORT_H */
