/* Version defines */

#ifndef _VERS_ID
#define _VERS_ID

#define __stringize2(X) #X
#define __stringize(X) __stringize2(X)

#define D1X_RAYTRACER_VERSION_MAJOR __stringize(D1X_RAYTRACER_VERSION_MAJORi)
#define D1X_RAYTRACER_VERSION_MINOR __stringize(D1X_RAYTRACER_VERSION_MINORi)
#define D1X_RAYTRACER_VERSION_MICRO __stringize(D1X_RAYTRACER_VERSION_MICROi)

#define BASED_VERSION "Registered v1.5 Jan 5, 1996"
#define VERSION D1X_RAYTRACER_VERSION_MAJOR "." D1X_RAYTRACER_VERSION_MINOR "." D1X_RAYTRACER_VERSION_MICRO
#define DESCENT_VERSION "D1X Raytracer " VERSION

#endif /* _VERS_ID */
