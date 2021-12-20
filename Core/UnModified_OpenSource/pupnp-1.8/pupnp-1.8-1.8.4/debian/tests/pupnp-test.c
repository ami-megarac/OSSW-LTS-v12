// pupnp smoketest

#include <stdio.h>
#include <upnp.h>

int main(void)
{
    int init_result = UpnpInit2(NULL, 0);
    if (init_result != UPNP_E_SUCCESS)
    {
        printf("Error initializing UPnP: %d\n", init_result);
        return 1;
    }

    printf("Listening on %s:%d\n", UpnpGetServerIpAddress(), UpnpGetServerPort());

    int term_result = UpnpFinish();
    if (term_result != UPNP_E_SUCCESS)
    {
        printf("Error terminating UPnP: %d\n", term_result);
        return 1;
    }

    return 0;
}
