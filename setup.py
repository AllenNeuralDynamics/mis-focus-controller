import setuptools

with open("README.md", "r", newline="", encoding="utf-8") as fh:
    long_description = fh.read()

setuptools.setup(
    name="multicam_focus",
    version="0.0.1",
    author="Joshua Vasquez",
    author_email="joshua.vasquez@alleninstitute.org",
    description="a library for controlling the multicam focus rig",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/AllenNeuralDynamics/mis-multicam-focus",
    packages=setuptools.find_packages(),
    classifiers=[
        "Development Status :: 4 - Beta",
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: MIT License",
        "Operating System :: POSIX :: Linux",
    ],
    python_requires='>=3.6',
    install_requires=['pyserial']
)
