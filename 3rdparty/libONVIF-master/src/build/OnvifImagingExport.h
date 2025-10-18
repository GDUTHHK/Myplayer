
#ifndef ONVIFIMAGING_EXPORT_H
#define ONVIFIMAGING_EXPORT_H

#ifdef ONVIFIMAGING_STATIC_DEFINE
#  define ONVIFIMAGING_EXPORT
#  define ONVIFIMAGING_NO_EXPORT
#else
#  ifndef ONVIFIMAGING_EXPORT
#    ifdef onvifimaging_EXPORTS
        /* We are building this library */
#      define ONVIFIMAGING_EXPORT __declspec(dllexport)
#    else
        /* We are using this library */
#      define ONVIFIMAGING_EXPORT __declspec(dllimport)
#    endif
#  endif

#  ifndef ONVIFIMAGING_NO_EXPORT
#    define ONVIFIMAGING_NO_EXPORT 
#  endif
#endif

#ifndef ONVIFIMAGING_DEPRECATED
#  define ONVIFIMAGING_DEPRECATED __declspec(deprecated)
#endif

#ifndef ONVIFIMAGING_DEPRECATED_EXPORT
#  define ONVIFIMAGING_DEPRECATED_EXPORT ONVIFIMAGING_EXPORT ONVIFIMAGING_DEPRECATED
#endif

#ifndef ONVIFIMAGING_DEPRECATED_NO_EXPORT
#  define ONVIFIMAGING_DEPRECATED_NO_EXPORT ONVIFIMAGING_NO_EXPORT ONVIFIMAGING_DEPRECATED
#endif

/* NOLINTNEXTLINE(readability-avoid-unconditional-preprocessor-if) */
#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef ONVIFIMAGING_NO_DEPRECATED
#    define ONVIFIMAGING_NO_DEPRECATED
#  endif
#endif

#endif /* ONVIFIMAGING_EXPORT_H */
