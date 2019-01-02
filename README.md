## Notes encryption application for Sailfish OS

![icon](icons/86x86/harbour-foilnotes.png)

**Foil Notes** allows you to encrypt some notes from Sailfish
OS Notes database with a password stronger than the lock code.
Strictly speaking, each note is encrypted with a unique random
256-bit AES key which in turn is encrypted with an RSA key which
in turn is encrypted with your password. If the bad guys get your
encrypted notes, they would have to crack the AES key (different
for each note) or the RSA key (shared by all notes but harder to
crack) in order to extract the content. If they get the encrypted
RSA key as well, then they can brute force your password. So in
the end, the encryption is as strong as your password.

The format of the encrypted file is described
[here](https://github.com/monich/foil/blob/master/libfoilmsg/README).
