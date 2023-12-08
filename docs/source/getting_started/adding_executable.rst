Executables
===========

Executable binaries for training, demonstration, or development-testing purposes should be added as subdirectories to the ``/src/executables`` directory. For an easy way to get started, follow these steps:

1. **Duplicate Directory**: Duplicate the ``src/executables/executable_example`` folder, along with its entire contents, and choose an appropriate name for the new directory. The new directory should be, for example, ``src/executables/your_new_directory_name``.

2. **Include the New Directory**: In ``src/executables/CMakeLists.txt``, add a line to include the newly created directory. Replace ``your_new_directory_name`` with the name you just chose.
    .. code-block:: cmake

        add_subdirectory(executable_example) # Existing
        add_subdirectory(your_new_directory_name) # Added
    

3. In ``src/executables/your_new_directory_name/CMakeLists.txt``, change the ``target_name`` as follows:
    .. code-block:: cmake

        # set(targetname executable_example) # Remove
        set(targetname your_new_directory_name) # Add
    

Rerunning cmake should enable you to select the target ``dp_your_new_directory_name``. Note that this target is excluded from all, meaning that you have to specifically select it to compile and run.
