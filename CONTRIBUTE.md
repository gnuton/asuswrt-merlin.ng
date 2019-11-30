
## How can I contribute?
Support asus merlin, send me patch if it's just for DSL-AC68U, tell me you are interested in this project by starring/forking it on github.

### Why do you use circleCI to build this firmware?
Because we are in 201x and Circle CI is an excellent CI solution which builds and create releases almost automatically and in a trasparent way.

### Why do you build This firmware in a docker image?
Because it makes the build faster since it doesn't need to install toolchains every time and because it makes the build reproducible everywhere even not in the cloud.

### Hey man I forked your repo and...
#### How can I build Asus Merlin for DSL-AC68U or other routers by myself?
TODO

#### Hey man the code is not up-to-date, how can I update it by myself?
In case I have not done it yet, ping me or just please feel free to do it and send me a PR.

#### BTW Here are the steps to update the merlin upstream code and the GPL pre-builds and files:
1. Merge the Merlin.ng mainline in the dsl-ac68u branch using git
```bash
   ========================
   // Clone gnuton repo and setup upstream
   git clone git@github.com:gnuton/asuswrt-merlin.ng.git
   cd asuswrt-merlin.ng/
   git remote add upstream git@github.com:RMerl/asuswrt-merlin.ng.git
   git fetch upstream

   // Update mainline branch
   git checkout -b mainline origin/mainline
   git rebase upstream/mainline
   git push origin


   // Update DSL
   git checkout -b dsl-ac68u origin/dsl-ac68u
   git branch dsl-ac68u-test
   git checkout dsl-ac68u-test
   git merge mainline
   // Fix conflicts if any
   git commit -m "a message" -a
   // Push to the cloud and let the CI build it for you
   git push origin dsl-ac68u-test
   // if everything fine merge the changes to dsl-ac68u branch
   git checkout dsl-ac68u
   git rebase dsl-ac68u-test
   git push --delete origin dsl-ac68u-test && git branch -d dsl-ac68u-test
   ========================
```
2. Download the latest DSL-AC68U firmware from https://www.asus.com/Networking/DSLAC68U/HelpDesk_Download/
3. uncompress it
4. cd dir_where_asuswrt_gpl_code_is
5. export REPO_TO_UPDATE=path_to_dsl-ac68u_git_working_copy
6. run $REPO_TO_UPDATE/tools/copy-prebuilt DSL-AC68U
7. git commit just the modified files belonging to DSL-AC68U
8. build and test it

#### I setup circleci for my repo how can I create releases?
CircleCI has bee configured so that it create release for stable releases and pre-releases for unstable ones.

##### Unstable builds or pre-releases
CircleCI builds snapshots from commit tagged with gnuton-snapshot-.*. So if you add a new feature that you wanna test, you can build the pre-release with

```bash
# Create and push tag -> this trigger the pre-release creation
git tag gnuton-snapshot-my-feature
git push --tag

# If you make some changes and you wanna update the pre-release
... make your changes and commit them ...
git push
git tag gnuton-snapshot-my-feature -f
git push --tag -f

# when you do not need the release/tag animore you can remove it from github and from the repo
git tag -d gnuton-snapshot-my-feature
git push --delete origin gnuton-snapshot-my-feature
```
##### Stable builds or releases
Stable builds are triggered by tags matching this pattern. [0-9]+.*gnuton. (eg: 380.0-gnuton1)
```
// To create and push the tag in order to build the release
git tag 390.0-gnuton1
git push --tag
```
### Where can I find official GPL Asus code?
You can download it from [here](https://www.asus.com/uk/Networking/DSLAC68U/HelpDesk_Download/)
