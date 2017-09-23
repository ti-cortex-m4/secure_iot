Example Summary
----------------
This application demonstrates the use of TLS/SSL to connect an IoT product to a
cloud service provider like Exosite.

Build Details - WolfSSL library
-------------------------------
This application uses WolfSSL libraries (with support for TM4C hardware
ciphers) for TLS functionality. Read the following wiki for details on
downloading and installing TI-RTOS and WolfSSL along with detailed instructions
on building WolfSSL libraries for TI-RTOS:
    http://processors.wiki.ti.com/index.php/Using_wolfSSL_with_TI-RTOS

TI-RTOS v2.14.00.10 or later and WolfSSL v3.6.6 or later are needed for this
application to work successfully.

Build Details - Application
---------------------------
Before building the application:
    Change the code in "cloud_task.h" marked by "USER STEP" as needed.

After building WolfSSL for TI-RTOS:
    Add '<WolfSSL_Installation>' path to the compiler include path.
    Add the WolfSSL library with support for TM4C hardware ciphers to the
    linker options. This library is located at
    '<WolfSSL_Installation>/tirtos/packages/ti/net/wolfSSL/lib/wolfssl_tm4c_hw.a<target>'
    Build the application.

Certificate generation
----------------------
This example needs a root certificate to build. A root certificate is already
included with the attached project. The root certificate has to be a C header
file containing byte buffer of the public key.

The steps to regenerate a "certificate.h" are documented below. The generated
file should be placed next to the "cloud_task.c" file for the application to
build.

Download "Equifax Secure Certificate Authority" certificate (.pem format) from:
    https://www.geotrust.com/resources/root-certificates/

Rename the file to "ca_cert.pem".

Generate the "certificate.h" from the downloaded certificate using the
following command:

    <xdctools_dir>/xs --xdcpath=<tirtos_dir>/packages ti.net.tools.gencertbuf --certs "ca_cert.pem"

    where,
          <xdctools_dir> with XDCtools installation directory path.
          <tirtos_dir> with TI-RTOS insallation directory path.

Example Usage
-------------
This application records various board activity by a user and periodically
reports it to a cloud server managed by Exosite.  In order to run this example
successfully, you will need to have an account with Exosite and will also have
to register the board (that is being used) to your Exosite profile with its
original factory provided MAC address.  The steps to create an account and
register the board are provided on the URL http://ti.exosite.com.  This
information is also provided in the Quickstart document that is shipped with
the EK-TM4C129EXL evaluation kit.

While running the application, the device must be connected to a network with a
DHCP server and internet connection.

The example starts the network stack and the WolfSSL stack.  When the network
stack receives an IP address from the DHCP server, it is printed to the
console.  A new task called the Cloud Task is stated to manage access to the
cloud server.

The cloud task syncs time with an NTP server, which is needed to validate
server certificate during HTTPS handshake process.  The NTP server IP address
is got by resolving the URL time.nist.gov, which resolves to a new IP every
time.  Once the time is synced, the cloud task attempts to connect to the
Exosite server. If a valid CIK is not found in EEPROM, the cloud task requests
a new CIK.  The CIK is needed for communicating with the Exosite server.  On
possesing a valid CIK, the cloud task continuously writes to and reads from the
Exosite server once every second using HTTPS POST and GET requests.

A command task manages all access to UART0 including a command-line based
interface to send commands to the EK-TM4C129EXL board. To access the UART0
console use the settings 115200-8-N-1.  On the console, print "help" for a list
of commands.

If your local internet connection requires the use of a proxy server, you can
enter the proxy server setting using the command-line interface with the
command "proxy help".

Additional Information
----------------------
For additional details on TI-RTOS, refer to the TI-RTOS web page at:
http://www.ti.com/tool/ti-rtos

For additional details on WolfSSL, refer to the WolfSSL web site at:
https://wolfssl.com
