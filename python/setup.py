from setuptools import setup, find_packages
from pathlib import Path


def find_stubs(path: Path):
    return [str(pyi.relative_to(path)) for pyi in path.rglob("*.pyi")]


setup(
    name='dp',
    version='0.1',
    packages=find_packages(),
    package_data={"dp": [*find_stubs(path=Path("dp"))]},
    include_package_data=True
)

