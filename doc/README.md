## Building the documentation locally

Run the following commands once for the initial setup:

- Make sure nodejs (LTS version >= 16.0.0) & npm are installed: `sudo apt install nodejs npm`
- Make sure you are in the doc folder: `cd doc`
- Run the Make target for the setup: `make setup`

The setup creates a pseudo-repository in the doc folder:
Antora requires the doc sources to be located within a git repository, but it cannot recognize git submodules. By setting up a pseudo-repository, the local documentation build works when MSM is used as a standalone repo as well as when MSM is opened from a submodule path within the Boost super-project.

After the initial setup is done, build the the documentation with `make build`.

If you are not interested in viewing the generated API reference, you can speed up the documenation build with the ENV `ANTORA_SKIP_CPP_REFERENCE=1`.
