# Third party notices

SQLite amalgamation: supplied in the original project; SQLite authors dedicate SQLite to the public domain. See the comments in include/sqlite3/sqlite3.c.

Catch2 2.13.10: Copyright Catch2 authors, distributed under the Boost Software License 1.0. Original single header: https://github.com/catchorg/Catch2/tree/v2.13.10. See tests/vendor/LICENSE.txt.

Windows BCrypt is an operating-system library. This project uses it for random salt generation only. Its existing password hash is a custom non-cryptographic algorithm and must be replaced before real deployment.
