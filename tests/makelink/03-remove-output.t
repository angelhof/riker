Move to test directory
  $ cd $TESTDIR

Clean up any leftover state
  $ rm -rf .dodo
  $ rm -f output A B

Set up the inptu file
  $ echo "Hello" > input

Run the build
  $ $DODO --show
  dodo-launch Dodofile
  Dodofile
  ln -s input A
  ln -s output B
  cat A
  unlink A
  unlink B

Make sure the output is in place
  $ cat output
  Hello

Now remove the output file
  $ rm output

Run a rebuild, which only needs to rerun cat
  $ $DODO --show
  cat A

Check the output
  $ cat output
  Hello

Make sure the A and B links do not exist
  $ stat A
  stat: cannot stat 'A': No such file or directory
  [1]

  $ stat B
  stat: cannot stat 'B': No such file or directory
  [1]

Clean up
  $ rm -rf .dodo
  $ rm -f output A B
