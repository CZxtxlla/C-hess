from setuptools import setup
from torch.utils.cpp_extension import BuildExtension, CppExtension

setup(
    name='chess_dataloader',
    ext_modules=[
        CppExtension(
            name='chess_dataloader', 
            sources=['dataloader_ext.cpp', 'dataloader.cpp'], # Add your C/C++ files here
            extra_compile_args=['-O3'] # Maximum C++ optimization
        )
    ],
    cmdclass={'build_ext': BuildExtension}
)