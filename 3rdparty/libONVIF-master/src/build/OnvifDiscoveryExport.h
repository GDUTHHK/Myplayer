
#ifndef ONVIFDISCOVERY_EXPORT_H
#define ONVIFDISCOVERY_EXPORT_H

#ifdef ONVIFDISCOVERY_STATIC_DEFINE
#  define ONVIFDISCOVERY_EXPORT
#  define ONVIFDISCOVERY_NO_EXPORT
#else
#  ifndef ONVIFDISCOVERY_EXPORT
#    ifdef onvifdiscovery_EXPORTS
        /* We are building this library */
#      define ONVIFDISCOVERY_EXPORT __declspec(dllexport)
#    else
        /* We are using this library */
#      define ONVIFDISCOVERY_EXPORT __declspec(dllimport)
#    endif
#  endif

#  ifndef ONVIFDISCOVERY_NO_EXPORT
#    define ONVIFDISCOVERY_NO_EXPORT 
#  endif
#endif

#ifndef ONVIFDISCOVERY_DEPRECATED
#  define ONVIFDISCOVERY_DEPRECATED __declspec(deprecated)
#endif

#ifndef ONVIFDISCOVERY_DEPRECATED_EXPORT
#  define ONVIFDISCOVERY_DEPRECATED_EXPORT ONVIFDISCOVERY_EXPORT ONVIFDISCOVERY_DEPRECATED
#endif

#ifndef ONVIFDISCOVERY_DEPRECATED_NO_EXPORT
#  define ONVIFDISCOVERY_DEPRECATED_NO_EXPORT ONVIFDISCOVERY_NO_EXPORT ONVIFDISCOVERY_DEPRECATED
#endif

/* NOLINTNEXTLINE(readability-avoid-unconditional-preprocessor-if) */
#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef ONVIFDISCOVERY_NO_DEPRECATED
#    define ONVIFDISCOVERY_NO_DEPRECATED
#  endif
#endif

#endif /* ONVIFDISCOVERY_EXPORT_H */
