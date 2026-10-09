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

Team members create the branch from the issue: **Create a branch** in the Development section of the sidebar. GitHub names it after the issue, for example `23-add-the-event-dispatcher`, and links it to the issue. Then fetch it locally:

```
git fetch origin
git switch 23-add-the-event-dispatcher
```

Other contributors fork the repository and create a branch with the same kind of name in their fork.

Code guidelines:

- The code must build and run on Windows and Linux. OS-specific code goes in `engine/src/platform/` only.
- Paths stored in files are relative and use `/`.
- Format the code with `.clang-format` before committing.
- Add unit tests when the code can be tested without a window.

## Pull requests

- Use the issue title as the pull request title, fill in the template and include `Closes #<issue number>`.
- CI must pass on Windows and Linux. For a first contribution from a fork, a maintainer approves the CI run.
- One approval is required. Pull requests are squash-merged, so each task becomes a single commit on `master`.

## Bugs and ideas

Open an issue with the **Bug** or **Task** template. A maintainer reviews it before it is added to the project.
