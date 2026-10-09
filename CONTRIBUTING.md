# Contributing

## Finding a task

Every task is an issue tracked in the organization's GitHub Project. Tasks in **Ready** can be picked up. Tasks in **Backlog** are not ready yet, either because they depend on unfinished work or because they are not fully described.

Each task has a size and a level:

- Size: **XS** up to 2 hours, **S** up to half a day, **M** 1 to 2 days.
- Level: `good first issue` needs little context, `help wanted` requires knowing the module, `advanced` involves tricky design and is reviewed by an experienced member.

## Claiming a task

- **Team members** assign the issue to themselves and move it to **In progress**.
- **Other contributors** leave a comment on the issue. A maintainer assigns them and updates the status. Please wait for this before starting, so that two people do not work on the same thing.

Ask questions in the issue comments. If you cannot finish a task, unassign yourself and say what was done.

## Working on a task

Team members create a branch in this repository:

```
git switch master
git pull
git switch -c feature/CORE-15-events
```

Other contributors fork the repository and create the branch in their fork. Use `fix/short-description` for bug fixes.

Code guidelines:

- The code must build and run on Windows and Linux. OS-specific code goes in `Engine/src/Platform/` only.
- Paths stored in files are relative and use `/`.
- Format the code with `.clang-format` before committing.
- Add unit tests when the code can be tested without a window.

## Pull requests

- Fill in the pull request template and include `Closes #<issue number>`.
- CI must pass on Windows and Linux. For a first contribution from a fork, a maintainer approves the CI run.
- One approval is required. Pull requests are squash-merged, so each task becomes a single commit on `master`.

## Bugs and ideas

Open an issue with the **Bug** or **Task** template. A maintainer reviews it before it is added to the project.
