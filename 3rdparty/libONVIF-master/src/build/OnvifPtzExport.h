
#ifndef ONVIFPTZ_EXPORT_H
#define ONVIFPTZ_EXPORT_H

#ifdef ONVIFPTZ_STATIC_DEFINE
#  define ONVIFPTZ_EXPORT
#  define ONVIFPTZ_NO_EXPORT
#else
#  ifndef ONVIFPTZ_EXPORT
#    ifdef onvifptz_EXPORTS
        /* We are building this library */
#      define ONVIFPTZ_EXPORT __declspec(dllexport)
#    else
        /* We are using this library */
#      define ONVIFPTZ_EXPORT __declspec(dllimport)
#    endif
#  endif

#  ifndef ONVIFPTZ_NO_EXPORT
#    define ONVIFPTZ_NO_EXPORT 
#  endif
#endif

#ifndef ONVIFPTZ_DEPRECATED
#  define ONVIFPTZ_DEPRECATED __declspec(deprecated)
#endif

#ifndef ONVIFPTZ_DEPRECATED_EXPORT
#  define ONVIFPTZ_DEPRECATED_EXPORT ONVIFPTZ_EXPORT ONVIFPTZ_DEPRECATED
#endif

#ifndef ONVIFPTZ_DEPRECATED_NO_EXPORT
#  define ONVIFPTZ_DEPRECATED_NO_EXPORT ONVIFPTZ_NO_EXPORT ONVIFPTZ_DEPRECATED
#endif

/* NOLINTNEXTLINE(readability-avoid-unconditional-preprocessor-if) */
#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef ONVIFPTZ_NO_DEPRECATED
#    define ONVIFPTZ_NO_DEPRECATED
#  endif
#endif

#endif /* ONVIFPTZ_EXPORT_H */
