import fs from "node:fs/promises";
import { Workbook } from "@oai/artifact-tool";

const path = process.argv[2] ?? "E:/log_3.csv";
const csvText = await fs.readFile(path, "utf8");
const workbook = await Workbook.fromCSV(csvText, { sheetName: "Log" });
const overview = await workbook.inspect({
  kind: "workbook,sheet,table",
  maxChars: 2500,
  tableMaxRows: 4,
  tableMaxCols: 12,
});

const lines = csvText.trim().split(/\r?\n/);
const header = lines[0].split(",");
const index = Object.fromEntries(header.map((name, i) => [name, i]));
const rows = lines.slice(1).map((line) => line.split(","));
const malformed = rows.filter((row) => row.length !== header.length).length;
const num = (row, name) => Number(row[index[name]]);
const validNums = (name) => rows.map((r) => num(r, name)).filter(Number.isFinite);
const stats = (values) => {
  if (!values.length) return null;
  const sorted = [...values].sort((a, b) => a - b);
  const sum = values.reduce((a, b) => a + b, 0);
  return {
    min: sorted[0],
    mean: sum / values.length,
    median: sorted[Math.floor(sorted.length / 2)],
    p95: sorted[Math.floor((sorted.length - 1) * 0.95)],
    max: sorted.at(-1),
  };
};

const timestamps = validNums("timestamp_ms");
const intervals = timestamps.slice(1).map((v, i) => v - timestamps[i]).filter((v) => v >= 0);
const validityColumns = header.filter((h) => h.endsWith("_ok") || h.endsWith("_valid"));
const validity = Object.fromEntries(validityColumns.map((name) => [
  name,
  rows.length ? validNums(name).filter((v) => v !== 0).length / rows.length : 0,
]));

const gnssReceived = index.gnss_received === undefined ? [] : validNums("gnss_received");
const gnssChecksum = index.gnss_checksum_ok === undefined ? [] : validNums("gnss_checksum_ok");
const gnssRmcNonEmpty = index.gnss_gnrmc === undefined ? 0 : rows.filter((r) => (r[index.gnss_gnrmc] ?? "").length > 0).length;
const gnssUpdates = [];
let previousRmc = null;
if (index.gnss_gnrmc !== undefined) {
  for (const row of rows) {
    const rmc = row[index.gnss_gnrmc] ?? "";
    if (rmc && rmc !== previousRmc) {
      gnssUpdates.push(num(row, "timestamp_ms"));
      previousRmc = rmc;
    }
  }
}
const gnssUpdateIntervals = gnssUpdates.slice(1).map((v, i) => v - gnssUpdates[i]);
const report = {
  artifactOverview: overview.ndjson,
  rows: rows.length,
  columns: header.length,
  malformedRows: malformed,
  durationSeconds: timestamps.length > 1 ? (timestamps.at(-1) - timestamps[0]) / 1000 : 0,
  timestampIntervalMs: stats(intervals),
  effectiveHz: intervals.length ? 1000 / (intervals.reduce((a, b) => a + b, 0) / intervals.length) : 0,
  sampleProcessingMs: index.sample_processing_ms === undefined ? null : stats(validNums("sample_processing_ms")),
  logDropMax: index.log_drop_count === undefined ? null : Math.max(...validNums("log_drop_count")),
  validity,
  gnss: {
    receivedRows: gnssReceived.filter((v) => v !== 0).length,
    checksumOkRows: gnssChecksum.filter((v) => v !== 0).length,
    rmcNonEmptyRows: gnssRmcNonEmpty,
    uniqueUpdates: gnssUpdates.length,
    updateIntervalMs: stats(gnssUpdateIntervals),
    updateHz: gnssUpdateIntervals.length
      ? 1000 / (gnssUpdateIntervals.reduce((a, b) => a + b, 0) / gnssUpdateIntervals.length)
      : 0,
    ageMs: index.gnss_age_ms === undefined ? null : stats(validNums("gnss_age_ms")),
  },
  airspeed: index.airspeed_mps === undefined ? null : stats(validNums("airspeed_mps")),
  airspeedTx: index.airspeed_tx_mps === undefined ? null : stats(validNums("airspeed_tx_mps")),
  firstTimestamp: timestamps[0],
  lastTimestamp: timestamps.at(-1),
  header,
};
console.log(JSON.stringify(report, null, 2));
