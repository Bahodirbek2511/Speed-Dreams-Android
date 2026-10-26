# Phone APK development signing key

The adjacent `speed-dreams-phone-debug.keystore.b64` is a **public test/development signing key** used so successive sideloaded builds retain the same Android signing identity. It is deliberately not a production or Play Store key. The repository is public, and anyone can recover this key from the base64 file.

Do not use this key for Google Play distribution, sensitive apps, or any app that relies on signature-level permissions. A production release must use a private signing key stored in a protected GitHub Actions secret or other secure signing service.

The CI workflow decodes this file to `$HOME/.android/debug.keystore`, where the upstream Gradle signing configuration expects it. The local test-key password and alias are the standard development values: store/key password `android`, alias `androiddebugkey`.
