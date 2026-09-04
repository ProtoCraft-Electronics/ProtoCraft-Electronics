// Root CA certificate for script.google.com (Google Apps Script), used to
// verify the server's identity over HTTPS. This is the "do it properly"
// alternative to WiFiClientSecure::setInsecure(), which is what most
// ESP32-to-Google-Sheets tutorials use - setInsecure() skips certificate
// validation entirely, which works, but means the connection can't tell a
// real Google server from anyone impersonating one.
//
// This file is intentionally NOT filled in with a certificate. Copying a
// blob of base64 from a video description and trusting it is exactly the
// habit this is trying to avoid. Get the real, current one yourself:
//
//   openssl s_client -connect script.google.com:443 -showcerts </dev/null
//
// That prints the certificate chain presented by the server. Take the
// LAST certificate in the output (the root), including the
// -----BEGIN CERTIFICATE----- and -----END CERTIFICATE----- lines, and
// paste it below.
//
// Root certificates are long-lived (years, not months) but do eventually
// get replaced. If cloud logging that was working suddenly starts failing
// with a TLS/handshake error in the Serial Monitor, this is the first
// thing to regenerate.
//
// Copy this file to google_root_ca.h and paste your certificate in.

const char* GOOGLE_ROOT_CA = R"EOF(
-----BEGIN CERTIFICATE-----
PASTE THE CERTIFICATE YOU RETRIEVED HERE
-----END CERTIFICATE-----
)EOF";
