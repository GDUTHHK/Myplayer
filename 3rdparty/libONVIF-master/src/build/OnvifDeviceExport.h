
#ifndef ONVIFDEVICE_EXPORT_H
#define ONVIFDEVICE_EXPORT_H

#ifdef ONVIFDEVICE_STATIC_DEFINE
#  define ONVIFDEVICE_EXPORT
#  define ONVIFDEVICE_NO_EXPORT
#else
#  ifndef ONVIFDEVICE_EXPORT
#    ifdef onvifdevice_EXPORTS
        /* We are building this library */
#      define ONVIFDEVICE_EXPORT __declspec(dllexport)
#    else
        /* We are using this library */
#      define ONVIFDEVICE_EXPORT __declspec(dllimport)
#    endif
#  endif

#  ifndef ONVIFDEVICE_NO_EXPORT
#    define ONVIFDEVICE_NO_EXPORT 
#  endif
#endif

#ifndef ONVIFDEVICE_DEPRECATED
#  define ONVIFDEVICE_DEPRECATED __declspec(deprecated)
#endif

#ifndef ONVIFDEVICE_DEPRECATED_EXPORT
#  define ONVIFDEVICE_DEPRECATED_EXPORT ONVIFDEVICE_EXPORT ONVIFDEVICE_DEPRECATED
#endif

#ifndef ONVIFDEVICE_DEPRECATED_NO_EXPORT
#  define ONVIFDEVICE_DEPRECATED_NO_EXPORT ONVIFDEVICE_NO_EXPORT ONVIFDEVICE_DEPRECATED
#endif

/* NOLINTNEXTLINE(readability-avoid-unconditional-preprocessor-if) */
#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef ONVIFDEVICE_NO_DEPRECATED
#    define ONVIFDEVICE_NO_DEPRECATED
#  endif
#endif

#endif /* ONVIFDEVICE_EXPORT_H */
