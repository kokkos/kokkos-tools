# Developer's Guide to Contributing to Kokkos Tools



## Contributing a simple change to kokkos-tools 

### Step 0: Confirm that this fix is established as needed

Before starting with contributing a change to the kokkos-tools repo, one must ask the following questions. 

Let's say you have a fix for bug of an off-by-one error in the kp_kernel_logger printing which indices of an array a particular thread is running.  

Before starting with contributing a change to the kokkos-tools repo, one must ask the following questions: 

1. Is the off-by-one error in an actual github issue? Is there a github issue on the repo ( seen in the Issues tab on the page https://github.com/kokkos/kokkos-tools) which identifies a problem/bug/enhanchment/feature to kokkos-tools, associated with this fix? 
 
2. Is that off-by-one error well-defined and discussed by at least one member of the key POCs of kokkos-tools and Kokkos PI listed in the README? For the problem that your proposed fix is solving, is that problem well-defined and discussed on the Kokkos github issue? 

3. Is your fix only making a change to an off-by-one error? Or, do you also improve the printing and display, e.g., use a tab instead of space, between the start and end index of an array that a thread executes? Generally: Is your fix associated with just one github issue on kokkos tools? 

-----------

### Step 1: Create a fork Kokkos-tools  (create a copy you own based on the latest version)  

* First, open up a browser and point it to github.com/kokkos/kokkos-tools

* Then, fork the repo github.com/kokkos/kokkos-tools as 

    github.com/<YOURGITHUBID>/kokkos-tools 

* After this, Open up a Terminal and find a location for your fork of kokkos-tools, and then set the environment variable Kokkos_TOOLS_LIB to that location. 

* Finally, clone the repo in the location in 2   

 git clone https://github.com/<YOURGITHUBID>/kokkos-tools 

* NOTE: Be sure to use/checkout the develop branch (this will be default soon and master won't be available - instead we will have releases) 

  ` git checkout develop `

* Do a `git status` to make sure you have the latest develop 
 
  
  ---- 
  
### Step 2: Make a version of your copy that has your proposed fix
6. Create new branch with your changes for fix: 

 git checkout -b myFix 

As mentioned in Step 0, note that: 

* The fix should be linked to a kokkos-tools github issue, i.e.,  the fix should solve a defined problem put forth and agreed upon on github.com/kokkos/kokkos-tools

* The fix should only have one change. 
 
  
  -------

### Step 3: Make the changes and test them 

* Open up the vi editor, and make changes to <yourfilename>. Vi is suggested as it is readily available on nearly any platform, and Kokkos development happens across different platforms.

* Use clang-format8 on the file to comply with formatting that git will perform.

### Step 4: Test the code works

* Compile and run the code with changes on platforms of importance, making sure no errors exist. 

* (Optional): use clang-format-8 on <yourfilename> to ensure code formatting of CMakeLists.txt before committing 


-----
  
### Step 4: et ready for submitting your changes to your version with fix 

* Do a git diff to review all the changes you have made to <yourfilename> 

* Stage the changes for commit 

`git add <filename>` 


* Commit the changes 
 
 `git commit <filename>`   


* Put in comments on changes to the commit in vi editor

-------

### Step 5: Request your fix be integrated into kokkos-tools
* Push the changes to file name myFile 


 `git push` 


* On github.com/<yourGITHUBID>/kokkos-tools, switch to branch <myChange>

* Open up a PR against in two ways: 

   a. Suggest a PR against kokkos's kokkos-tools develop branch; or

   b. the develop branch of kokkos-tools 

  
-----------

 
