This directory contains an example of the type of database one can use to
store personal licenses, so that VitaShell can automatically restore them
when reinstalling content from PKG data.

As opposed to this sample, which does not contain any usable data, your
CONTENT_ID columns should contain valid content id for the PS Vita app,
game or DLC you own, and the RIF columns should contain a 512-byte .rif
blob that includes your AID, a CONTENT_ID that matches the one from the
first column as well as your personal license data.

To edit this database manually, you can use the Windows/Linux/MacOS 
compatible DB Browser for SQLite (http://sqlitebrowser.org/) to drag and
drop your .rif files and edit the relevant CONTENT_ID fields.

Or you can wait until someone writes an application that automates that
process or you... ;)

This database should be copied to ux0:license/