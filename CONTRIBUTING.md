# Contributing

Tasks are GitHub issues, tracked in the [project board](https://github.com/orgs/ultirisstudio/projects/3). The commands below use [Git](https://git-scm.com) and the [GitHub CLI](https://cli.github.com). Replace `42` with the number of your issue.

## 1. Get the code

```
gh repo clone ultirisstudio/QuasarEngine -- --recursive
cd QuasarEngine
```

If you are not a member of the organization, fork the repository instead:

```
gh repo fork ultirisstudio/QuasarEngine --clone -- --recursive
```

## 2. Pick a task

Choose a task in the **Ready** column that nobody is assigned to. If you are new to the project, look for the `good first issue` label.

```
gh issue view 42 --repo ultirisstudio/QuasarEngine
```

## 3. Take it

Assign it to yourself and set its status to **In progress** on the project board:

```
gh issue edit 42 --repo ultirisstudio/QuasarEngine --add-assignee @me
```

Not a member? Leave a comment on the issue instead, and wait for a maintainer to assign you.

## 4. Create the branch

```
gh issue develop 42 --repo ultirisstudio/QuasarEngine --checkout
```

This creates a branch named after the issue, links it to the issue and switches to it. In a fork, create the branch yourself:

```
git switch -c 42-short-description
```

## 5. Work and push

```
git add -A
git commit -m "Core: Add the event dispatcher"
git push -u origin HEAD
```

Use the issue title as the commit message.

## 6. Open the pull request

```
gh pr create --title "Core: Add the event dispatcher"
```

Fill in the template and keep the `Closes #42` line, so the issue is closed when the pull request is merged. Then set the task to **In review**.

CI must pass on Windows and Linux, and one approval is needed before merging. To answer review comments, commit and push again on the same branch.

## Questions, bugs and ideas

Ask questions in the comments of the issue. To report a bug or propose a task, open an issue with the **Bug** or **Task** template.
