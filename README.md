# C_remote_server
Simple example of Robot Framework remote server in C

1. Take http://xmlrpc-c.sourceforge.net/ and build (configure, make, make install).
2. If complaining about missing config.h, copy it to this project
3. Compile remote server using command: gcc -o remote xmlrpc_remote_server.c -Wl,--no-as-needed `xmlrpc-c-config abyss-server --libs`
4. export LD_LIBRARY_PATH=/usr/local/lib
5. Run with command: ./remote 8072
6. Run included robot test with command: robot robotTest.txt
