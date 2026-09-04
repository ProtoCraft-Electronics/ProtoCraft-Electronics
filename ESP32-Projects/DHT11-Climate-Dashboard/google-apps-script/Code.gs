// Paste this into Extensions > Apps Script on a blank Google Sheet.
//
// Deploy > New deployment > type "Web app" > Execute as "Me" > Who has
// access "Anyone" > Deploy. Copy the URL it gives you into secrets.h as
// GOOGLE_SCRIPT_URL.
//
// If you edit this script later, you need to create a new deployment (or
// update the existing one under Deploy > Manage deployments) for the
// changes to actually take effect - saving alone isn't enough.

function doPost(e) {
  var sheet = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet();
  var data = JSON.parse(e.postData.contents);

  var date = new Date(data.ts * 1000);
  sheet.appendRow([date, data.temp, data.hum]);

  return ContentService
    .createTextOutput(JSON.stringify({ ok: true }))
    .setMimeType(ContentService.MimeType.JSON);
}
