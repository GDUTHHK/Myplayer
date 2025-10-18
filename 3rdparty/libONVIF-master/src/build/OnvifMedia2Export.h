
#ifndef ONVIFMEDIA2_EXPORT_H
#define ONVIFMEDIA2_EXPORT_H

#ifdef ONVIFMEDIA2_STATIC_DEFINE
#  define ONVIFMEDIA2_EXPORT
#  define ONVIFMEDIA2_NO_EXPORT
#else
#  ifndef ONVIFMEDIA2_EXPORT
#    ifdef onvifmedia2_EXPORTS
        /* We are building this library */
#      define ONVIFMEDIA2_EXPORT __declspec(dllexport)
#    else
        /* We are using this library */
#      define ONVIFMEDIA2_EXPORT __declspec(dllimport)
#    endif
#  endif

#  ifndef ONVIFMEDIA2_NO_EXPORT
#    define ONVIFMEDIA2_NO_EXPORT 
#  endif
#endif

#ifndef ONVIFMEDIA2_DEPRECATED
#  define ONVIFMEDIA2_DEPRECATED __declspec(deprecated)
#endif

#ifndef ONVIFMEDIA2_DEPRECATED_EXPORT
#  define ONVIFMEDIA2_DEPRECATED_EXPORT ONVIFMEDIA2_EXPORT ONVIFMEDIA2_DEPRECATED
#endif

#ifndef ONVIFMEDIA2_DEPRECATED_NO_EXPORT
#  define ONVIFMEDIA2_DEPRECATED_NO_EXPORT ONVIFMEDIA2_NO_EXPORT ONVIFMEDIA2_DEPRECATED
#endif

/* NOLINTNEXTLINE(readability-avoid-unconditional-preprocessor-if) */
#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef ONVIFMEDIA2_NO_DEPRECATED
#    define ONVIFMEDIA2_NO_DEPRECATED
#  endif
#endif

#endif /* ONVIFMEDIA2_EXPORT_H */
