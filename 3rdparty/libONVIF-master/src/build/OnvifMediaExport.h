
#ifndef ONVIFMEDIA_EXPORT_H
#define ONVIFMEDIA_EXPORT_H

#ifdef ONVIFMEDIA_STATIC_DEFINE
#  define ONVIFMEDIA_EXPORT
#  define ONVIFMEDIA_NO_EXPORT
#else
#  ifndef ONVIFMEDIA_EXPORT
#    ifdef onvifmedia_EXPORTS
        /* We are building this library */
#      define ONVIFMEDIA_EXPORT __declspec(dllexport)
#    else
        /* We are using this library */
#      define ONVIFMEDIA_EXPORT __declspec(dllimport)
#    endif
#  endif

#  ifndef ONVIFMEDIA_NO_EXPORT
#    define ONVIFMEDIA_NO_EXPORT 
#  endif
#endif

#ifndef ONVIFMEDIA_DEPRECATED
#  define ONVIFMEDIA_DEPRECATED __declspec(deprecated)
#endif

#ifndef ONVIFMEDIA_DEPRECATED_EXPORT
#  define ONVIFMEDIA_DEPRECATED_EXPORT ONVIFMEDIA_EXPORT ONVIFMEDIA_DEPRECATED
#endif

#ifndef ONVIFMEDIA_DEPRECATED_NO_EXPORT
#  define ONVIFMEDIA_DEPRECATED_NO_EXPORT ONVIFMEDIA_NO_EXPORT ONVIFMEDIA_DEPRECATED
#endif

/* NOLINTNEXTLINE(readability-avoid-unconditional-preprocessor-if) */
#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef ONVIFMEDIA_NO_DEPRECATED
#    define ONVIFMEDIA_NO_DEPRECATED
#  endif
#endif

#endif /* ONVIFMEDIA_EXPORT_H */
