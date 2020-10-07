Move to test directory
  $ cd $TESTDIR

Clean up any leftover state
  $ rm -rf .dodo hello

Copy in the original version of hello.c
  $ cp file_versions/hello-original.c hello.c

Touch the hello file for now, since make will stat it
  $ touch hello

Run the build
  $ $DODO --show
  dodo-launch Dodofile
  Dodofile
  make --always-make --quiet -j1
  gcc -o hello hello.c
  cc1 * (glob)
  as * (glob)
  collect2 * (glob)
  ld * (glob)

Run the hello executable
  $ ./hello
  Hello world

Run a rebuild, which should do nothing
  $ $DODO --show

Make sure the output still works
  $ ./hello
  Hello world

Clean up
  $ rm -rf .dodo hello
