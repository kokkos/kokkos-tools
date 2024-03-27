# Developer's Guide to Contributing to Kokkos Tools

## Summary 

The success of Kokkos Tools, a subproject of the Kokkos project, relies on open-source software development. Kokkos Tools sub-project has a mix of in-house contributors (those on the Kokkos team) and external contributors (those not on the Kokkos team). 

The purpose of this document is to educate those on the Kokkos team as well as contributors external to Kokkos team on how
to make code contributions to the Kokkos Tools repository. 

## Introduction 

Kokkos Tools is comprised of two categories of software components: 
(1) a Kokkos Tools infrastructure that hooks into the Kokkos core library; and
(2) a set of tool libraries, i.e., connectors (to the Kokkos Tools infrastructure 

Software component (1) above has been and continues to be developed in-house
This component includes CMakeLists.txt and README.md at the top-level directory, along with `Kokkos_Profiling.cpp` 
in Kokkos core. 

The open-source development, i.e., external contributions, of Kokkos Tools is expected to be primarily in (2). 

Within (2), about half of the connectors are from contributors outside of the Kokkos project. Such a balance of in-house versus open-source development 
is expected to be sustained throughout the life of the Kokkos Tools sub-project of the Kokkos project.

Given this mix of in-house and external contributions, a set of guidelines for contributing customized for Kokkos Tools is needed. 

## Contributing a simple change to kokkos-tools 

------------
### Step 0: Confirm that this fix is established as needed

Before starting with contributing a change to the Kokkos Tools repo, one must ask the following questions. 

Let's say you have a fix for bug of an off-by-one error in the kp_kernel_logger that results in it printing the wrong kernel ID.  

Before starting with contributing a change to the kokkos-tools repo, one must ask the following questions: 

1. Is the problem defined in the github issue? Is there a github issue on the repo (seen in the Issues tab on the webpage https://github.com/kokkos/kokkos-tools, or by going directly to https://github.com/kokkos/kokkos-tools/issues) which identifies a problem/bug/enhanchment/feature to kokkos-tools, associated with this fix? 
 
2. Is that error well-defined and discussed by at least one member of the key POCs of kokkos-tools and Kokkos PI listed in the README? For the problem that your proposed fix is solving, is that problem well-defined and discussed on the Kokkos github issue? 

3. Is your fix only making a change to an off-by-one error? Or, do you also improve the printing and display, e.g., use a tab instead of space to when displaying the kernel ID? Generally: Is your fix associated with just one github issue on kokkos tools? 

-----------

### Step 1: Create a fork Kokkos-tools  (create a copy you own based on the latest version)  

1. First, open up a browser and point it to github.com/kokkos/kokkos-tools

1. Then, fork the repo github.com/kokkos/kokkos-tools as 

    github.com/<YOURGITHUBID>/kokkos-tools 

1. Open up a Terminal on your computer and find a location for your fork of kokkos-tools, and then set the environment variable `Kokkos_TOOLS_LIBS` to that location. 

1. Clone the repo in the location in 2

 `git clone https://github.com/<YOURGITHUBID>/kokkos-tools .`
 
1. Do a checkout of the `develop` branch. This branch is the default on the kokkos project's Kokkos Tools repo. 
     a. Note 1: your `master` branch is irrelevant here for development. That is designated as a stable release branch. 
     a. Note 2: You should synch your fork's `develop` branch periodically with the Kokkos project's Kokkos Tools repo. 

  `git checkout develop`

1. Do a `git status` to make sure you have the latest `develop` branch from the remote, i.e., the version showing up on github.com/<YOURGITHUBID>/kokkos-tools in a browser. 
  
  ---- 
  
### Step 2: Make a version of your copy that has your proposed fix
6. Create new branch with your changes for fix: 

 `git checkout -b myFix` 

As mentioned in Step 0, note that: 

* The fix should be linked to a kokkos-tools github issue, i.e., the fix should solve a defined problem put forth and agreed upon on github.com/kokkos/kokkos-tools

* The fix should only have one functional modification (i.e., multiple changes , or edits to a file, can be made, but those changes should all be towards fixing exactly on github Issue. 
 
  
  -------

### Step 3: Make the changes and test them 

* The preferred editor for Kokkos Tools development is vi. Open up a vi editor, and make changes to `<yourfilename>`. 
    - The editor `vi` is suggested to use as it is readily available on nearly any platform, and Kokkos development happens across different platforms.


### Step 4: Test the code works

1. Compile and run the code with changes on platforms of importance, making sure no errors exist.
2. Use `clang-format-8` on the file to comply with formatting that git will perform.
 use `clang-format-8` on `<yourfilename>` to ensure code formatting of your code files before committing. If on a mac, you can obtain clang-format by downloading homebrew if you don't have it and the using homebrew to install clang-format-8 by typing `brew install clang-format-8`. 


-----
  
### Step 4: get ready for submitting your changes to your version with fix 

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
 
 * Put a meaningful description of the PR, and include the github Issue the PR is fixing. 
 
 * Assign yourself to the PR as well as any others you think are relevant. 
 
 * Put in reviewers for the PR

 * if you are not fully ready to have others review (e.g., if you 
 want the reviewers to see that you have a draft getting ready and that a review is needed soon), then submit a draft pull request 
 Otherwise, submit the PR for review. 
 
 * Message individuals on Kokkosteam.slack.com about the PR to promote its timely review. You can also directly ping Vivek Kale on that slack along with Christian Trott on that slack to get it moving.
 
 * Once your PR approved, you must merge the PR into Kokkos-tools:develop. Again, note that kokkos-tools only has a `develop` branch, and the kokkosteam only provides periodic releases of kokkos-tools via merging `develop` branch into the `master` branch.
  
-----------

 
