
#ifndef ONVIFPROBE_EXPORT_H
#define ONVIFPROBE_EXPORT_H

#ifdef ONVIFPROBE_STATIC_DEFINE
#  define ONVIFPROBE_EXPORT
#  define ONVIFPROBE_NO_EXPORT
#else
#  ifndef ONVIFPROBE_EXPORT
#    ifdef onvifprobe_EXPORTS
        /* We are building this library */
#      define ONVIFPROBE_EXPORT __declspec(dllexport)
#    else
        /* We are using this library */
#      define ONVIFPROBE_EXPORT __declspec(dllimport)
#    endif
#  endif

#  ifndef ONVIFPROBE_NO_EXPORT
#    define ONVIFPROBE_NO_EXPORT 
#  endif
#endif

#ifndef ONVIFPROBE_DEPRECATED
#  define ONVIFPROBE_DEPRECATED __declspec(deprecated)
#endif

#ifndef ONVIFPROBE_DEPRECATED_EXPORT
#  define ONVIFPROBE_DEPRECATED_EXPORT ONVIFPROBE_EXPORT ONVIFPROBE_DEPRECATED
#endif

#ifndef ONVIFPROBE_DEPRECATED_NO_EXPORT
#  define ONVIFPROBE_DEPRECATED_NO_EXPORT ONVIFPROBE_NO_EXPORT ONVIFPROBE_DEPRECATED
#endif

/* NOLINTNEXTLINE(readability-avoid-unconditional-preprocessor-if) */
#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef ONVIFPROBE_NO_DEPRECATED
#    define ONVIFPROBE_NO_DEPRECATED
#  endif
#endif

#endif /* ONVIFPROBE_EXPORT_H */
