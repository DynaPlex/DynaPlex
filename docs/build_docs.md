# Local building

For locally building the docs page you need to install a few unique dependencies that are listed in the `pyproject.toml` file.

The easiest way to install these dependencies is using `poetry`. After installing `poetry`, use `poetry install --with docs` in the `docs` folder. 

Next, you can run `poetry run make html` to build and display the docs in a static `.html` file.

Finally, all template settings are in `conf.py`.