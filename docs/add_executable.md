# Executables

Executable binaries for training, demonstration or development-testing purposes should be added as subdirectories to the `/src/examples`  directory. For an easy way to get started, simply
1. **Duplicate Directory**: duplicate the `src/examples/executable_example` folder, with the entire contents, and choose an appropriate name for the new directory. 
2. **Include the new directory**: In `src/examples/CMakeLists.txt`, add a line to include the newly created directory. (`your_new_directory_name` should be replaced with the name you just chose.)
    ```cmake
    add_subdirectory(executable_example)#existing
    add_subdirectory(your_new_directory_name)#added   
    ```
3. in `src/examples/your_new_directory_name/CMakeLists.txt`, change the target_name as follows:
    ```cmake
    #set(targetname executable_example) # remove
    set(targetname your_new_directory_name) #add
    ```
Rerunning cmake should enable you to select the target `dp_your_new_directory_name`. Note that this targets is excluded from all, meaning that you have to specifically select it to compile and run. 