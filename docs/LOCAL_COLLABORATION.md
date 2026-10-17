# Local network collaboration

Open View → Local collaboration. Host starts a listener on this computer; Join
connects directly to the host's LAN address and displayed port using its session
ticket. Share the ticket with participants. No connection or listener starts at
application startup, and no account, cloud storage or external server is used.
LAN transport is unencrypted. Disconnect leaves the current local project open.

The session shares board sketches, components, parameters, wires and wire bends.
Every participant saves to their own local project path. Joining opens the shared
project as an unnamed local project; it does not overwrite the previously opened
file. Opening a different project or example disconnects the current session.
Libraries are installed separately on each computer. Local library installations,
paths and bundled library archives are not transferred. Received code is never
compiled or executed automatically.

Independent code and circuit changes merge against their common base. New
component and wire identities have a random participant namespace; wire identities
survive project save/load. Text merging combines separate changed ranges. Two
overlapping edits or ambiguous insertions are explicit conflicts; complex edits
may conservatively require resolution. There is no silent last-writer overwrite.

A conflict keeps your editor's local content. Compare drafts shows the current
local and host documents. Accept host version adopts the host state. Apply my
conflicting changes explicitly chooses your version at the conflicting fields
and retains independent host changes. Invalid circuits remain conflicts until
corrected. Before either choice a local `.thinkcad` draft is saved in the
`collaboration-drafts` subfolder of ThinkCad's recovery directory; its path is
printed in Output. The latest 20 conflict drafts are retained separately from
ordinary recovery snapshots. They can be opened with File → Open project.

Structural and code updates received during simulation are queued until Stop.
They do not modify a running circuit or restart sketches. Local source updates
use range edits when source tabs stay the same, preserving caret and undo state
where possible. Incoming structural changes clear circuit undo history so an old
whole-circuit undo cannot silently remove another participant's work.

Settings → Collaboration controls the peer limit (1–8), batching interval
(100–1000 ms) and host port (0 chooses an available port). Peer limit and port
apply to the next session. The interval applies to subsequent edits immediately.
Documents are limited to 4 MiB; incoming/outgoing buffers and message rates are
bounded. The host retains at most 32 revisions / 16 MiB of merge history. A base
older than that is rejected for explicit rebasing, preserving the local draft.
The local circuit limits of 100 components and 300 wires still apply.
