/**

  @file    tls/arduino/osal_tls_wifi.c
  @brief   OSAL TLS sockets API Arduino WiFi implementation.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    20.1.2017

  Implementation of OSAL sockets for secure TLS sockets over WiFi within Arduino framework.

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
/* Force tracing on for this source file.
 */
// #undef OSAL_TRACE
// #define OSAL_TRACE 3

#include "eosalx.h"
#if OSAL_TLS_SUPPORT
#if OSAL_OPENSSL_SUPPORT==0

#include <Arduino.h>
#include <WiFiClientSecure.h>

/* Global network setup. Micro-controllers typically have one (or two)
   network interfaces. The network interface configuration is managed
   here, not by operating system.
 */
static osalNetworkInterface osal_net_iface
  = {"BRASS",              /* host_name */
     "192.168.1.201",      /* ip_address */
     "255.255.255.0",      /* subnet_mask */
     "192.168.1.254",      /* gateway_address */
     "8.8.8.8",            /* dns_address */
     "66-7F-18-67-A1-D3",  /* mac */
     0};                   /* dhcp */


// www.howsmyssl.com root certificate authority, to verify the server
// change it to your server root CA

/* Certificate authority (trusted god) for Alice, Bob and Carol */
static const os_char test_root_ca[] = \
    "-----BEGIN CERTIFICATE-----\n"
    "MIIGOTCCBCGgAwIBAgIJAOE/vJd8EB24MA0GCSqGSIb3DQEBBQUAMIGyMQswCQYD\n"
    "VQQGEwJGUjEPMA0GA1UECAwGQWxzYWNlMRMwEQYDVQQHDApTdHJhc2JvdXJnMRgw\n"
    "FgYDVQQKDA93d3cuZnJlZWxhbi5vcmcxEDAOBgNVBAsMB2ZyZWVsYW4xLTArBgNV\n"
    "BAMMJEZyZWVsYW4gU2FtcGxlIENlcnRpZmljYXRlIEF1dGhvcml0eTEiMCAGCSqG\n"
    "SIb3DQEJARYTY29udGFjdEBmcmVlbGFuLm9yZzAeFw0xMjA0MjcxMDE3NDRaFw0x\n"
    "MjA1MjcxMDE3NDRaMIGyMQswCQYDVQQGEwJGUjEPMA0GA1UECAwGQWxzYWNlMRMw\n"
    "EQYDVQQHDApTdHJhc2JvdXJnMRgwFgYDVQQKDA93d3cuZnJlZWxhbi5vcmcxEDAO\n"
    "BgNVBAsMB2ZyZWVsYW4xLTArBgNVBAMMJEZyZWVsYW4gU2FtcGxlIENlcnRpZmlj\n"
    "YXRlIEF1dGhvcml0eTEiMCAGCSqGSIb3DQEJARYTY29udGFjdEBmcmVlbGFuLm9y\n"
    "ZzCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAODp+8oQcK+MTuWPZVxJ\n"
    "ZR75paK4zcUngupYXWSGWFXPTV7vssFk6vInePArTL+T9KwHfiZ29Pp3UbzDlysY\n"
    "Kz9f9Ae50jGD6xVPwXgQ/VI979GyFXzhiEMtSYykF04tBJiDl2/FZxbHPpNxC39t\n"
    "14kwuDqBin9N/ZbT5+45tbbS8ziXS+QgL5hD2q2eYCWayrGEt1Y+jDAdHDHmGnZ8\n"
    "d4hbgILJAs3IInOCDjC4c1gwHFb8G4QHHTwVhjhqpkq2hQHgzWBC1l2Dku/oDYev\n"
    "Zu/pfpTo3z6+NOYBrUWseQmIuG+DGMQA9KOuSQveyTywBm4G4vZKn0sCu1/v2+9T\n"
    "BGv41tgS/Yf6oeeQVrbS4RFY1r9qTK6DW9wkTTesa4xoDKQrWjSJ7+aa8tvBXLGX\n"
    "x2xdRNWLeRMuGBSOihwXmDr+rCJRauT7pItN5X+uWNTX1ofNksQSUMaFJ5K7L0LU\n"
    "iQqU2Yyt/8UphdVZL4EFkGSA13UDWtb9mM1hY0h65LlSYwCchEphrtI9cuV+ITrS\n"
    "NcN6cP/dqDx1/jWd6dqjNu7+dugwX5elQS9uUYCFmugR5s1m2eeBg3QuC7gZLE0N\n"
    "NbgS7oSxKJe9KeOcw68jHWfBKsCfBfQ4fU2t/ntMybT3hCdEMQu4dgM5Tyw/UeFq\n"
    "0SaJyTl+G1bTzS0FW6uUp6NLAgMBAAGjUDBOMB0GA1UdDgQWBBQjbC09PildeLhs\n"
    "Pqriuy4ebIfyUzAfBgNVHSMEGDAWgBQjbC09PildeLhsPqriuy4ebIfyUzAMBgNV\n"
    "HRMEBTADAQH/MA0GCSqGSIb3DQEBBQUAA4ICAQCwRJpJCgp7S+k9BT6X3kBefonE\n"
    "EOYtyWXBPpuyG3Qlm1rdhc66DCGForDmTxjMmHYtNmAVnM37ILW7MoflWrAkaY19\n"
    "gv88Fzwa5e6rWK4fTSpiEOc5WB2A3HPN9wJnhQXt1WWMDD7jJSLxLIwFqkzpDbDE\n"
    "9122TtnIbmKNv0UQpzPV3Ygbqojy6eZHUOT05NaOT7vviv5QwMAH5WeRfiCys8CG\n"
    "Sno/o830OniEHvePTYswLlX22LyfSHeoTQCCI8pocytl7IwARKCvBgeFqvPrMiqP\n"
    "ch16FiU9II8KaMgpebrUSz3J1BApOOd1LBd42BeTAkNSxjRvbh8/lDWfnE7ODbKc\n"
    "b6Ad3V9flFb5OBZH4aTi6QfrDnBmbLgLL8o/MLM+d3Kg94XRU9LjC2rjivQ6MC53\n"
    "EnWNobcJFY+soXsJokGtFxKgIx8XrhF5GOsT2f1pmMlYL4cjlU0uWkPOOkhq8tIp\n"
    "R8cBYphzXu1v6h2AaZLRq184e30ZO98omKyQoQ2KAm5AZayRrZZtjvEZPNamSuVQ\n"
    "iPe3o/4tyQGq+jEMAEjLlDECu0dEa6RFntcbBPMBP3wZwE2bI9GYgvyaZd63DNdm\n"
    "Xd65m0mmfOWYttfrDT3Q95YP54nHpIxKBw1eFOzrnXOqbKVmJ/1FDP2yWeooKVLf\n"
    "KvbxUcDaVvXB0EU0bg==\n"
    "-----END CERTIFICATE-----\n";

/* Bob's key */
static const os_char bobs_key[] = \
    "-----BEGIN RSA PRIVATE KEY-----\n"
    "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\n"
    "-----END RSA PRIVATE KEY-----\n";

/* Bob's key */
static const os_char bobs_certificate[] = 
    "-----BEGIN CERTIFICATE-----\n"
    "MIIGJTCCBA2gAwIBAgIBAjANBgkqhkiG9w0BAQUFADCBsjELMAkGA1UEBhMCRlIx\n"
    "DzANBgNVBAgMBkFsc2FjZTETMBEGA1UEBwwKU3RyYXNib3VyZzEYMBYGA1UECgwP\n"
    "d3d3LmZyZWVsYW4ub3JnMRAwDgYDVQQLDAdmcmVlbGFuMS0wKwYDVQQDDCRGcmVl\n"
    "bGFuIFNhbXBsZSBDZXJ0aWZpY2F0ZSBBdXRob3JpdHkxIjAgBgkqhkiG9w0BCQEW\n"
    "E2NvbnRhY3RAZnJlZWxhbi5vcmcwHhcNMTIwNDI3MTA1NDQwWhcNMjIwNDI1MTA1\n"
    "NDQwWjB8MQswCQYDVQQGEwJGUjEPMA0GA1UECAwGQWxzYWNlMRgwFgYDVQQKDA93\n"
    "d3cuZnJlZWxhbi5vcmcxEDAOBgNVBAsMB2ZyZWVsYW4xDDAKBgNVBAMMA2JvYjEi\n"
    "MCAGCSqGSIb3DQEJARYTY29udGFjdEBmcmVlbGFuLm9yZzCCAiIwDQYJKoZIhvcN\n"
    "AQEBBQADggIPADCCAgoCggIBAMI/QxRK1N1DWrlDXi27iaEXGPeuR0t69NTco+G3\n"
    "hToQIOu8URjYiyXGBJVPgOkFXAD0fCN70a2BWPGdQ8M37n9hA7X/KbsQGvuod5eb\n"
    "3kx9P8r/U4w3MLaI8g6+fNySdslfIpYZC5HqnBiWn0PRnSKe2cMSn4AFhR9wu4dd\n"
    "Y8FaUT1+aT12bbBW6ts/rvDNDBlIsfLVLuf6Et0VvIzcCcImnNwiUo7IHMHNAb0a\n"
    "JMW+TxgI895ZHI9jpmMdT1qSaHpJlCZU0YO+FuRej3MvgTo6MID9V6l/G3vlD2wB\n"
    "aPcfRUn+BjwIV2QnpQtVGLcwvghFcIvNQ+r8gB4DXMNSjalVU1X0YS6LUGRqMKdv\n"
    "vbiAEu5mmNh4X6D1ZWpt9QnMYk1VVoAhdUhzTbnj+R2WySxdeU08xXqehP+dx5SH\n"
    "Cj5pgdJ/wF9nnAaMM1yjn1LnBMfTge+ydx7QVx8fkKVpwA1DxfamfvfqRXxgtmgf\n"
    "ZFncYDPCE4y3BsIqzcwrAt6i6XAM23n+zutewAbrdkMJ4CrH7h5qr2BJczyoU4zh\n"
    "OSznnv79RCDwhZof68dAyFuQQ+ahagBQS3NzcsU5dxMePJW+qTdq0U40PTTsh/ge\n"
    "bOfci3+O0Tx4wuIJk9fAaK5wgbnw0PcmpOLAEh0vAWPrUwXLqttmsPsWm+fnvsNm\n"
    "2lzJAgMBAAGjezB5MAkGA1UdEwQCMAAwLAYJYIZIAYb4QgENBB8WHU9wZW5TU0wg\n"
    "R2VuZXJhdGVkIENlcnRpZmljYXRlMB0GA1UdDgQWBBSc0nFQNfcQQ93oznUpo1Nd\n"
    "EaeoOzAfBgNVHSMEGDAWgBQjbC09PildeLhsPqriuy4ebIfyUzANBgkqhkiG9w0B\n"
    "AQUFAAOCAgEAw7CkgvVk5U6g5XRexD3QnPdO942viy6AWWO1bi8QW2bWKSrK4gEg\n"
    "aOEr/9bh4fKm4Mz1j59ccrj6gXZ9XO5gKeXX3o9KnFU+5Sccdrw15xaAbzJ3/Veu\n"
    "UYf7vsKhzHaaYQHJ/4YA/9GWzf8sD0ieroPY39R4HUw3h/VYXSbGyhbN+hYdb0Ku\n"
    "V0qZRVKAXBx2Qqj48xWcGz42AeAJXtgZse2g7zvHCaeqX7YtwSCEmyyHGis13p6c\n"
    "DNkMXs9RONbWgK6RFbXGIt9+F5/D67/91TtL6mYAcqC1t2WoWtmo8WfBQdh53cwv\n"
    "eHqeXgqddw5ZUknSEJQc6/Q8BA48HBp1pugj1fBzFJCxcVoyV40012ph3HMa2h0f\n"
    "Vqcu7w2k9fuUC/TPHdIQDwfNup14h+gEY2rlemsgvb0pwjlb/IaEdwvj+Cw3rK8b\n"
    "7U+51gijrC8xB0r4js8R3ZIcyarHpbdipHduWCB4F8te721B67bCH3+h3vq7cZIg\n"
    "3rFeNIRs7WzhQ4YT8D/XLcW6wN43jUi838dPs6al5cLb8e/bDCVp5liNunK9Xj/P\n"
    "gTa2q+6oZ4/uu/5vyR+KH+/pyXpSQK2gPyNFemOVmD0SuOLzC4gQOARosPGni9Bh\n"
    "1w8vzxdRIet2aS0Z6AHFM/1hzUZkh4lD6THQvoigooIMf59mQTqaWmo=\n"
    "-----END CERTIFICATE-----\n";

/* static const char* test_root_ca = \
     "-----BEGIN CERTIFICATE-----\n" \
     "MIIEkjCCA3qgAwIBAgIQCgFBQgAAAVOFc2oLheynCDANBgkqhkiG9w0BAQsFADA/\n" \
     "MSQwIgYDVQQKExtEaWdpdGFsIFNpZ25hdHVyZSBUcnVzdCBDby4xFzAVBgNVBAMT\n" \
     "DkRTVCBSb290IENBIFgzMB4XDTE2MDMxNzE2NDA0NloXDTIxMDMxNzE2NDA0Nlow\n" \
     "SjELMAkGA1UEBhMCVVMxFjAUBgNVBAoTDUxldCdzIEVuY3J5cHQxIzAhBgNVBAMT\n" \
     "GkxldCdzIEVuY3J5cHQgQXV0aG9yaXR5IFgzMIIBIjANBgkqhkiG9w0BAQEFAAOC\n" \
     "AQ8AMIIBCgKCAQEAnNMM8FrlLke3cl03g7NoYzDq1zUmGSXhvb418XCSL7e4S0EF\n" \
     "q6meNQhY7LEqxGiHC6PjdeTm86dicbp5gWAf15Gan/PQeGdxyGkOlZHP/uaZ6WA8\n" \
     "SMx+yk13EiSdRxta67nsHjcAHJyse6cF6s5K671B5TaYucv9bTyWaN8jKkKQDIZ0\n" \
     "Z8h/pZq4UmEUEz9l6YKHy9v6Dlb2honzhT+Xhq+w3Brvaw2VFn3EK6BlspkENnWA\n" \
     "a6xK8xuQSXgvopZPKiAlKQTGdMDQMc2PMTiVFrqoM7hD8bEfwzB/onkxEz0tNvjj\n" \
     "/PIzark5McWvxI0NHWQWM6r6hCm21AvA2H3DkwIDAQABo4IBfTCCAXkwEgYDVR0T\n" \
     "AQH/BAgwBgEB/wIBADAOBgNVHQ8BAf8EBAMCAYYwfwYIKwYBBQUHAQEEczBxMDIG\n" \
     "CCsGAQUFBzABhiZodHRwOi8vaXNyZy50cnVzdGlkLm9jc3AuaWRlbnRydXN0LmNv\n" \
     "bTA7BggrBgEFBQcwAoYvaHR0cDovL2FwcHMuaWRlbnRydXN0LmNvbS9yb290cy9k\n" \
     "c3Ryb290Y2F4My5wN2MwHwYDVR0jBBgwFoAUxKexpHsscfrb4UuQdf/EFWCFiRAw\n" \
     "VAYDVR0gBE0wSzAIBgZngQwBAgEwPwYLKwYBBAGC3xMBAQEwMDAuBggrBgEFBQcC\n" \
     "ARYiaHR0cDovL2Nwcy5yb290LXgxLmxldHNlbmNyeXB0Lm9yZzA8BgNVHR8ENTAz\n" \
     "MDGgL6AthitodHRwOi8vY3JsLmlkZW50cnVzdC5jb20vRFNUUk9PVENBWDNDUkwu\n" \
     "Y3JsMB0GA1UdDgQWBBSoSmpjBH3duubRObemRWXv86jsoTANBgkqhkiG9w0BAQsF\n" \
     "AAOCAQEA3TPXEfNjWDjdGBX7CVW+dla5cEilaUcne8IkCJLxWh9KEik3JHRRHGJo\n" \
     "uM2VcGfl96S8TihRzZvoroed6ti6WqEBmtzw3Wodatg+VyOeph4EYpr/1wXKtx8/\n" \
     "wApIvJSwtmVi4MFU5aMqrSDE6ea73Mj2tcMyo5jMd6jmeWUHK8so/joWUoHOUgwu\n" \
     "X4Po1QYz+3dszkDqMp4fklxBwXRsW10KXzPMTZ+sOPAveyxindmjkW8lGy+QsRlG\n" \
     "PfZ+G6Z6h7mjem0Y+iWlkYcV4PIWL1iwBi8saCbGS5jN2p8M+X+Q7UNKEkROb3N6\n" \
     "KOqkqm57TH2H3eDJAkSnh6/DNFu0Qg==\n" \
     "-----END CERTIFICATE-----\n";
*/

// You can use x.509 client certificates if you want
//const char* test_client_key = "";   //to verify the client
//const char* test_client_cert = "";  //to verify the client

/** TLS library initialized flag.
 */
os_boolean osal_tls_initialized = OS_FALSE;

/** WiFi network connected flag.
 */
static os_boolean osal_wifi_initialized;

/** WiFi network connection timer.
 */
static os_timer osal_wifi_init_timer;

/** Arduino specific socket class to store information.
 */
class osalSocket
{
    public:
    /** A stream structure must start with this generic stream header structure, which contains
        parameters common to every stream.
     */
    osalStreamHeader hdr;

    /* Arduino librarie's Wifi TLS socket client object.
     */
    WiFiClientSecure client;

    /** Nonzero if socket is used.
     */
    os_boolean used;
};

/** Maximum number of sockets.
 */
#define OSAL_MAX_SOCKETS 8

/* Array of socket structures for every possible WizNet sockindex
 */
static osalSocket osal_tls[OSAL_MAX_SOCKETS];


/* Prototypes for forward referred static functions.
 */
static osalSocket *osal_get_unused_socket(void);

static os_boolean osal_is_wifi_initialized(
    void);

/**
****************************************************************************************************

  @brief Open a socket.
  @anchor osal_tls_open

  The osal_tls_open() function opens a socket. The socket can be either listening TCP
  socket, connecting TCP socket or UDP multicast socket.

  @param  parameters Socket parameters, a list string or direct value.
          Address and port to connect to, or interface and port to listen for.
          Socket IP address and port can be specified either as value of "addr" item
          or directly in parameter sstring. For example "192.168.1.55:20" or "localhost:12345"
          specify IPv4 addressed. If only port number is specified, which is often
          useful for listening socket, for example ":12345".
          IPv4 address is automatically recognized from numeric address like
          "2001:0db8:85a3:0000:0000:8a2e:0370:7334", but not when address is specified as string
          nor for empty IP specifying only port to listen. Use brackets around IP address
          to mark IPv6 address, for example "[localhost]:12345", or "[]:12345" for empty IP.

  @param  option Not used for sockets, set OS_NULL.

  @param  status Pointer to integer into which to store the function status code. Value
          OSAL_SUCCESS (0) indicates success and all nonzero values indicate an error.
          See @ref osalStatus "OSAL function return codes" for full list.
          This parameter can be OS_NULL, if no status code is needed.

  @param  flags Flags for creating the socket. Bit fields, combination of:
          - OSAL_STREAM_CONNECT: Connect to specified socket port at specified IP address.
          - OSAL_STREAM_LISTEN: Open a socket to listen for incoming connections.
          - OSAL_STREAM_UDP_MULTICAST: Open a UDP multicast socket.
          - OSAL_STREAM_NO_SELECT: Open socket without select functionality.
          - OSAL_STREAM_SELECT: Open socket with select functionality.
          - OSAL_STREAM_TCP_NODELAY: Disable Nagle's algorithm on TCP socket.
          - OSAL_STREAM_NO_REUSEADDR: Disable reusability of the socket descriptor.
          - OSAL_STREAM_BLOCKING: Open socket in blocking mode.

          See @ref osalStreamFlags "Flags for Stream Functions" for full list of stream flags.

  @return Stream pointer representing the socket, or OS_NULL if the function failed.

****************************************************************************************************
*/
osalStream osal_tls_open(
    const os_char *parameters,
    void *option,
    osalStatus *status,
    os_int flags)
{
    os_int port_nr;
    os_char host[OSAL_HOST_BUF_SZ];
    os_boolean is_ipv6;
    osalSocket *w;
    osalStatus rval = OSAL_STATUS_FAILED;

    /* Initialize sockets library, if not already initialized.
     */
    if (!osal_tls_initialized)
    {
        osal_tls_initialize(OS_NULL, 0, OS_NULL);
    }

    /* If WiFi network is not connected, we can do nothing.
     */
    if (!osal_is_wifi_initialized())
    {
        rval = OSAL_STATUS_PENDING;
        goto getout;
    }

    /* Get host name or numeric IP address and TCP port number from parameters.
       The host buffer must be released by calling os_free() function,
       unless if host is OS_NULL (unpecified).
     */
    osal_socket_get_host_name_and_port(parameters,
        &port_nr, host, sizeof(host), &is_ipv6, flags, IOC_DEFAULT_TLS_PORT);

    /* Get first unused osal_tls structure.
     */
    w = osal_get_unused_socket();
    if (w == OS_NULL)
    {
        osal_debug_error("osal_tls: Too many sockets");
        goto getout;
    }

    /* Set up client certificate, if we use one.
     */
      // w->client.setCACert(test_root_ca);  // XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX WE MAY WANT TO USE THIS
      // w->client.setCertificate(bobs_key); // for client verification
      // w->client.setPrivateKey(bobs_certificate);	// for client verification

    osal_trace2_int("Connecting to TLS socket port ", port_nr);
    osal_trace2(host);

    /* Connect the socket.
     */
    if (!w->client.connect(host, port_nr))
    {
        osal_trace("Wifi: TLS socket connect failed");
        w->client.stop();
        goto getout;
    }

    os_memclear(&w->hdr, sizeof(osalStreamHeader));
    w->used = OS_TRUE;
    w->hdr.iface = &osal_tls_iface;

    osal_trace2("wifi: TLS socket connected.");

    /* Success. Set status code and return socket structure pointer
       casted to stream pointer.
     */
    if (status) *status = OSAL_SUCCESS;
    return (osalStream)w;

getout:
    /* Set status code and return NULL pointer to indicate failure.
     */
    if (status) *status = rval;
    return OS_NULL;
}


/**
****************************************************************************************************

  @brief Close socket.
  @anchor osal_tls_close

  The osal_tls_close() function closes a socket, which was creted by osal_tls_open()
  function. All resource related to the socket are freed. Any attemp to use the socket after
  this call may result crash.

  @param   stream Stream pointer representing the socket. After this call stream pointer will
           point to invalid memory location.
  @return  None.

****************************************************************************************************
*/
void osal_tls_close(
    osalStream stream)
{
    osalSocket *w;

    if (stream == OS_NULL) return;
    w = (osalSocket*)stream;
    if (!w->used)
    {
        osal_debug_error("osal_tls: Socket closed twice");
        return;
    }

    w->client.stop();
    w->used = OS_FALSE;
}


/**
****************************************************************************************************

  @brief Accept connection from listening socket.
  @anchor osal_tls_open

  The osal_tls_accept() function accepts an incoming connection from listening socket.

  @param   stream Stream pointer representing the listening socket.
  @param   status Pointer to integer into which to store the function status code. Value
           OSAL_SUCCESS (0) indicates that new connection was successfully accepted.
           The value OSAL_STATUS_NO_NEW_CONNECTION indicates that no new incoming
           connection, was accepted.  All other nonzero values indicate an error,
           See @ref osalStatus "OSAL function return codes" for full list.
           This parameter can be OS_NULL, if no status code is needed.
  @param   flags Flags for creating the socket. Define OSAL_STREAM_DEFAULT for normal operation.
           See @ref osalStreamFlags "Flags for Stream Functions" for full list of flags.
  @return  Stream pointer representing the socket, or OS_NULL if the function failed.

****************************************************************************************************
*/
osalStream osal_tls_accept(
    osalStream stream,
    osalStatus *status,
    os_int flags)
{
    if (status) *status = OSAL_STATUS_FAILED;
    return OS_NULL;
}


/**
****************************************************************************************************

  @brief Flush the socket.
  @anchor osal_tls_flush

  The osal_tls_flush() function flushes data to be written to stream.


  IMPORTANT, FLUSH MUST BE CALLED: The osal_stream_flush(<stream>, OSAL_STREAM_DEFAULT) must
  be called when select call returns even after writing or even if nothing was written, or
  periodically in in single thread mode. This is necessary even if no data was written
  previously, the socket may have stored buffered data to avoid blocking.

  @param   stream Stream pointer representing the socket.
  @param   flags See @ref osalStreamFlags "Flags for Stream Functions" for full list of flags.
  @return  Function status code. Value OSAL_SUCCESS (0) indicates success and all nonzero values
           indicate an error. See @ref osalStatus "OSAL function return codes" for full list.

****************************************************************************************************
*/
osalStatus osal_tls_flush(
    osalStream stream,
    os_int flags)
{
    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Write data to socket.
  @anchor osal_tls_write

  The osal_tls_write() function writes up to n bytes of data from buffer to socket.

  @param   stream Stream pointer representing the socket.
  @param   buf Pointer to the beginning of data to place into the socket.
  @param   n Maximum number of bytes to write.
  @param   n_written Pointer to integer into which the function stores the number of bytes
           actually written to socket,  which may be less than n if there is not enough space
           left in the socket. If the function fails n_written is set to zero.
  @param   flags Flags for the function.
           See @ref osalStreamFlags "Flags for Stream Functions" for full list of flags.
  @return  Function status code. Value OSAL_SUCCESS (0) indicates success and all nonzero values
           indicate an error. See @ref osalStatus "OSAL function return codes" for full list.

****************************************************************************************************
*/
osalStatus osal_tls_write(
    osalStream stream,
    const os_char *buf,
    os_memsz n,
    os_memsz *n_written,
    os_int flags)
{
    osalSocket *w;
    int bytes;

    *n_written = 0;

    if (stream == OS_NULL) return OSAL_STATUS_FAILED;
    w = (osalSocket*)stream;
    if (!w->used)
    {
        osal_debug_error("osal_tls: Unused socket");
        return OSAL_STATUS_FAILED;
    }

    if (!w->client.connected())
    {
        osal_debug_error("osal_tls: Not connected");
        return OSAL_STATUS_FAILED;
    }
    if (n == 0) return OSAL_SUCCESS;

    bytes = w->client.write(buf, n);
    if (bytes < 0) bytes = 0;
    *n_written = bytes;

#if OSAL_TRACE >= 3
    if (bytes > 0) osal_trace("Data written to socket");
#endif

    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Read data from socket.
  @anchor osal_tls_read

  The osal_tls_read() function reads up to n bytes of data from socket into buffer.

  @param   stream Stream pointer representing the socket.
  @param   buf Pointer to buffer to read into.
  @param   n Maximum number of bytes to read. The data buffer must large enough to hold
           at least this many bytes.
  @param   n_read Pointer to integer into which the function stores the number of bytes read,
           which may be less than n if there are fewer bytes available. If the function fails
           n_read is set to zero.
  @param   flags Flags for the function, use OSAL_STREAM_DEFAULT (0) for default operation.
           The OSAL_STREAM_PEEK flag causes the function to return data in socket, but nothing
           will be removed from the socket.
           See @ref osalStreamFlags "Flags for Stream Functions" for full list of flags.

  @return  Function status code. Value OSAL_SUCCESS (0) indicates success and all nonzero values
           indicate an error. See @ref osalStatus "OSAL function return codes" for full list.

****************************************************************************************************
*/
osalStatus osal_tls_read(
    osalStream stream,
    os_char *buf,
    os_memsz n,
    os_memsz *n_read,
    os_int flags)
{
    osalSocket *w;
    int bytes;

    *n_read = 0;

    if (stream == OS_NULL) return OSAL_STATUS_FAILED;
    w = (osalSocket*)stream;
    if (!w->used)
    {
        osal_debug_error("osal_tls: Socket can not be read");
        return OSAL_STATUS_FAILED;
    }

    if (!w->client.connected())
    {
        osal_debug_error("osal_tls: Not connected");
        return OSAL_STATUS_FAILED;
    }

    bytes = w->client.available();
    if (bytes < 0) bytes = 0;
    if (bytes)
    {
        if (bytes > n)
        {
            bytes = n;
        }

        bytes = w->client.read(buf, bytes);
    }

#if OSAL_TRACE >= 3
    if (bytes > 0) osal_trace2_int("Data received from socket", bytes);
#endif

    *n_read = bytes;
    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Get socket parameter.
  @anchor osal_tls_get_parameter

  The osal_tls_get_parameter() function gets a parameter value.

  @param   stream Stream pointer representing the socket.
  @param   parameter_ix Index of parameter to get.
           See @ref osalStreamParameterIx "stream parameter enumeration" for the list.
  @return  Parameter value.

****************************************************************************************************
*/
os_long osal_tls_get_parameter(
    osalStream stream,
    osalStreamParameterIx parameter_ix)
{
    /* Call the default implementation
     */
    return osal_stream_default_get_parameter(stream, parameter_ix);
}


/**
****************************************************************************************************

  @brief Set socket parameter.
  @anchor osal_tls_set_parameter

  The osal_tls_set_parameter() function gets a parameter value.

  @param   stream Stream pointer representing the socket.
  @param   parameter_ix Index of parameter to get.
           See @ref osalStreamParameterIx "stream parameter enumeration" for the list.
  @param   value Parameter value to set.
  @return  None.

****************************************************************************************************
*/
void osal_tls_set_parameter(
    osalStream stream,
    osalStreamParameterIx parameter_ix,
    os_long value)
{
    /* Call the default implementation
     */
    osal_stream_default_set_parameter(stream, parameter_ix, value);
}


/**
****************************************************************************************************

  @brief Get first unused osal_tls.
  @anchor osal_get_unused_socket

  The osal_get_unused_socket() function finds index of first unused osal_tls item in
  osal_tls array.

  @return Pointer to OSAL TLS socket structure, or OS_NULL if no free ones.

****************************************************************************************************
*/
static osalSocket *osal_get_unused_socket(void)
{
    int i;

    for (i = 0; i < OSAL_MAX_SOCKETS; i++)
    {
        if (!osal_tls[i].used) return osal_tls + i;
    }
    return OS_NULL;
}


/**
****************************************************************************************************

  @brief Convert string to binary MAC or IP address.
  @anchor osal_str_to_bin

  The osal_mac_from_str() converts string representation of MAC or IP address to binary.

  @param   x Pointer to byte array into which to store the address.
  @param   n Size of x in bytes. 4 or 6 bytes.
  @param   str Input, MAC or IP address as string.
  @param   c Separator character.
  @param   b 10 for decimal numbers (IP address) or 16 for hexadecimal numbers (MAC).
  @return  OS_TRUE if successfull.

****************************************************************************************************
*/
/* static int osal_str_to_bin(
    byte *x,
    os_short n,
    const os_char* str,
    os_char c,
    os_short b)
{
    os_int i;

    for (i = 0; i < n; i++)
    {
        x[i] = (byte)strtoul(str, NULL, b);
        str = strchr(str, c);
        if (str == NULL) break;
        ++str;
    }
    return i + 1 == n;
}
*/


/**
****************************************************************************************************

  @brief Convert string to binary IP address.
  @anchor osal_ip_from_str

  The osal_ip_from_str() converts string representation of IP address to binary.
  If the function fails, binary IP address is left unchanged.

  @param   ip Pointer to Arduino IP address to set.
  @param   str Input, IP address as string.
  @return  None.

****************************************************************************************************
*/
/* static void osal_ip_from_str(
    IPAddress& ip,
    const os_char *str)
{
    byte buf[4];
    os_short i;

    if (osal_str_to_bin(buf, sizeof(buf), str, '.', 10))
    {
        for (i = 0; i<(os_short)sizeof(buf); i++) ip[i] = buf[i];
    }
#if OSAL_DEBUG
    else
    {
        osal_debug_error("IP string error");
    }
#endif
}
*/


/**
****************************************************************************************************

  @brief Convert string to binary MAC address.
  @anchor osal_mac_from_str

  The osal_mac_from_str() converts string representation of MAC address to binary.
  If the function fails, binary MAC is left unchanged.

  @param   mac Pointer to byte array into which to store the MAC.
  @param   str Input, MAC address as string.
  @return  None.

****************************************************************************************************
*/
/* static void osal_mac_from_str(
    byte mac[6],
    const char* str)
{
    byte buf[6];

    if (osal_str_to_bin(buf, sizeof(buf), str, '-', 16))
    {
        os_memcpy(mac, buf, sizeof(buf));
    }
#if OSAL_DEBUG
    else
    {
        osal_debug_error("MAC string error");
    }
#endif
}
*/


/**
****************************************************************************************************

  @brief Initialize sockets LWIP/WizNet.
  @anchor osal_tls_initialize

  The osal_tls_initialize() initializes the underlying sockets library. This used either DHCP,
  or statical configuration parameters.

  @param   nic Pointer to array of network interface structures. Can be OS_NULL to use default.
           This implementation sets up only nic[0].
  @param   n_nics Number of network interfaces in nic array.
  @param   prm Parameters related to TLS. Can OS_NULL for client if not needed.

  @return  None.

****************************************************************************************************
*/
void osal_tls_initialize(
    osalNetworkInterface *nic,
    os_int n_nics,
    osalTLSParam *prm)
{
    const os_char *wifi_net_name = "bean24";
    const os_char *wifi_net_password = "talvi333";
    int i;

    /* Initialize only once.
     */
    /* IPAddress
        ip_address(192, 168, 1, 201),
        dns_address(8, 8, 8, 8),
        gateway_address(192, 168, 1, 254),
        subnet_mask(255, 255, 255, 0);

    byte
        mac[6] = {0x66, 0x7F, 0x18, 0x67, 0xA1, 0xD3};
    */

    /* Clear Get parameters. Use defaults if not set.
     */
    for (i = 0; i < OSAL_MAX_SOCKETS; i++)
    {
        osal_tls[i].used = OS_FALSE;
    }

    /* Get parameters. Use defaults if not set.
     */
    if (nic == OS_NULL) n_nics = 0;
    if (n_nics >= 1)
    {
        if (*nic->wifi_net_name != '\0') wifi_net_name = nic->wifi_net_name;
        if (*nic->wifi_net_password != '\0') wifi_net_password = nic->wifi_net_password;
    }

    osal_tls_initialized = OS_TRUE;

    // osal_mac_from_str(mac, osal_net_iface.mac);

    /* Initialize using static configuration.
    osal_ip_from_str(ip_address, osal_net_iface.ip_address);
    osal_ip_from_str(dns_address, osal_net_iface.dns_address);
    osal_ip_from_str(gateway_address, osal_net_iface.gateway_address);
    osal_ip_from_str(subnet_mask, osal_net_iface.subnet_mask);
     */

    /* Start the WiFi. Do not wait for the results here, we wish to allow IO to run even
       without WiFi network.
     */
    osal_trace("Commecting to Wifi network");
    osal_trace(wifi_net_name);
    WiFi.begin(wifi_net_name, wifi_net_password);

    /* Set TLS library initialized flag, now waiting for wifi initialization. We do not lock
     * the code here to allow IO sequence, etc to proceed even without wifi.
     */
    osal_tls_initialized = OS_TRUE;
    osal_wifi_initialized = OS_FALSE;
}


/**
****************************************************************************************************

  @brief Check if WiFi network is connected.
  @anchor osal_is_wifi_initialized

  The osal_is_wifi_initialized() function is used when opening or staring to listen for incoming
  connections to make sure that WiFi network is connected.

  @return  OS_TRUE if we are connected to WiFi network, or OS_FALSE otherwise.

****************************************************************************************************
*/
static os_boolean osal_is_wifi_initialized(
    void)
{
    if (!osal_tls_initialized) return OS_FALSE;
    if (!osal_wifi_initialized)
    {
        /* If WiFi is not connected, just return failure.
         */
        if (WiFi.status() != WL_CONNECTED)
        {
            if (os_elapsed(&osal_wifi_init_timer, 500))
            {
                osal_trace2("Waiting for wifi");
                os_get_timer(&osal_wifi_init_timer);
            }
            return OS_FALSE;
        }
        osal_trace("Wifi network connected");

        /* Here WE should convert IP address to string.
           ip_address = Ethernet.localIP(); */

        /* Mark that Wifi is intialized.
         */
        osal_wifi_initialized = OS_TRUE;
    }
    return OS_TRUE;
}


/**
****************************************************************************************************

  @brief Shut down sockets.
  @anchor osal_tls_shutdown

  The osal_tls_shutdown() shuts down the underlying sockets library.

  @return  None.

****************************************************************************************************
*/
void osal_tls_shutdown(
    void)
{
    if (osal_tls_initialized)
    {
        WiFi.disconnect();
        osal_tls_initialized = OS_FALSE;
    }
}


/**
****************************************************************************************************

  @brief Keep the sockets library alive.
  @anchor osal_tls_maintain

  The osal_tls_maintain() function should be called periodically to maintain sockets
  library.

  @return  None.

****************************************************************************************************
*/
void osal_tls_maintain(
    void)
{
//    Ethernet.maintain();
}


#if OSAL_FUNCTION_POINTER_SUPPORT

/** Stream interface for OSAL sockets. This is structure osalStreamInterface filled with
    function pointers to OSAL sockets implementation.
 */
const osalStreamInterface osal_tls_iface
 = {osal_tls_open,
    osal_tls_close,
    osal_tls_accept,
    osal_tls_flush,
    osal_stream_default_seek,
    osal_tls_write,
    osal_tls_read,
    osal_stream_default_write_value,
    osal_stream_default_read_value,
    osal_tls_get_parameter,
    osal_tls_set_parameter,
    OS_NULL};

#endif

#endif
#endif
