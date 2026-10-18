# Local update transactions

ThinkCad uses native Qt networking and widgets for release discovery, consent,
download and verification. `UpdateDownload` owns bounded streaming, a local
partial-file journal, a single-writer lock, pause/cancel/retry and SHA-256.
`Updater` owns release policy and provenance. `UpdateDialog` presents their state.
Closing the dialog hides it; the explicit Cancel action controls the operation.

Release metadata is bounded. Stable release names and public GitHub asset paths
are checked. Downloads are limited to 512 MiB, use HTTPS and accepted GitHub/CDN
hosts, and must match both release size and SHA-256. The loopback HTTP option
exists only as an explicit constructor dependency for local transport tests; the
production controller never enables it. Resuming checks Content-Range; a 200
response restarts the file. No combined bytes are trusted without hashing the
complete file. Three transient retries use 1/2/4-second waits. Explicitly discarded
partials are removed; keeping a partial is an intentional user choice.

The pinned local GitHub CLI verifies the bundle against the embedded trust root,
repository, workflow, `refs/heads/main`, and expected source commit. The Qt side
also requires a nonempty successful JSON verification result with that digest.
No invalid payload becomes installable. Credentials are not required or logged.

## Windows

The detached, statically linked helper is copied outside the installation. It:

1. Waits for ThinkCad to exit; another application instance must not be forcibly
   closed. The installer uses `/NOCLOSEAPPLICATIONS` and its application mutex.
2. Rechecks provenance with the same shared verification arguments used in the
   existing negative verification tests.
3. Locks the installation transaction, checks available space and rejects links
   or special files in the installation tree.
4. Writes a durable journal and temporary HKCU RunOnce recovery command.
5. Renames the old tree to an adjacent unique backup and clones its contents.
   Existing local extra files are therefore retained. Nothing outside this tree
   is copied, removed or rewritten by the helper.
6. Runs the signed/attested Inno installer in a process job. The progress callback
   reports actual Inno progress; extraction and replacement remain one phase.
7. Runs the installed application with `--smoke-test` and the expected version.
8. Records a receipt, commits the journal, removes RunOnce and restarts ThinkCad.

Cancel/timeout terminates the install process job before attempting rollback.
Rollback retains the failed tree separately and renames the backup into place;
it also restores the saved per-user uninstall registration when available.
Committed transactions never roll back merely because their RunOnce cleanup was
interrupted. Backups remain for manual inspection/removal. Disk space must cover
an extra installation tree; locked directories, antivirus and permission failures
can prevent restoration. In that case logs, backup and journal are retained, and
recovery can be retried after the lock is released. This is recovery where the
filesystem permits it, not a guarantee against disk failure or arbitrary user
changes during installation.

`Test-UpdateTransaction.ps1` exercises an actual verified installer with retained
local files on Windows CI. Qt tests exercise partial transfers, ignored/invalid
ranges, corrupt hashes, writer locks, retry limits, cancellation/restart and
filesystem transaction rollback/recovery. Windows verification rejection tests
continue to check tampering and wrong source identities.

## Linux

Distribution packages and Gentoo are updated by their package managers. The
portable x86_64 AppImage supports bounded verified download, a `.previous` backup,
atomic replacement, and version/smoke checks. Failed health checks restore the
backup. Atomic rename leaves a complete old or new image on interruption, but a
power failure after replacement may require manually launching/restoring
`.previous`. No system service or distribution ownership bypass is installed.

## Data and localization

Open projects must be saved before installation starts. Project formats do not
change. User settings, libraries, scripts and recovery stores outside the install
root remain untouched. Logs and receipts stay local. Release-note content is
shown as published, including known issues and project-format notes, without
inventing a machine-readable migration guarantee for releases without one.

New auxiliary translations were generated locally using the pinned tools/models
and attribution recorded in [the 1.3.1 architecture notes](development/1.3.1-architecture.md).
No model or translation service is a runtime dependency.
