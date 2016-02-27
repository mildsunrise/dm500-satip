var container = document.getElementById("container");

// Replaces #container contents with the rendered HTML
// Fields we could show: adapter.enabled, adapter.phyad, adapter.phyfd, adapter.bw, version
function render() {
  if (adapter.type !== 1) {
    container.innerHTML = '<p>Error: No tuner is available. Please <a href="https://github.com/mildsunrise/dm500-satip/issues/new">file an issue</a> on dm500-satip.</p>';
    return;
  }

  // FIXME: detect type correctly, using allsys
  //var types = [ "Unknown", "DVB-C", "DVB-CB", "DVB-T", "DSS", "DVB-S", "DVB-S2", "DVB-H", "ISDBT", "ISDBS", "ISDBC", "ATSC", "ATSCMH", "DMBTH", "CMMB", "DAB", "DVB-T2", "Turbo", "DVB-CC", "DVB-C2" ];
  var html = '<fieldset class="adapter"><h2>DVB-S adapter</h2><div>';

  // TUNING CARD

  html += '<div class="section"><h3><label>Tuned to</label></h3><div class="tuning-card">';

  html += '<p class="frequency" title="Frequency"><label>Frequency:</label>';
  if (adapter.freq) html += '<strong>' + adapter.freq + ' MHz</strong>';
  else html += 'Offline';
  html += '</p>';

  var polarization = [, "Vertical", "Horizontal"][adapter.pol]; //FIXME: include other two
  if (polarization === undefined) polarization = "Unknown";
  html += '<p class="polarization ' + polarization.toLowerCase() + '" title="Polarization"><label>Polarization:</label><strong>' + polarization + '</strong></p>';
  
  html += '<p class="symbol-rate" title="';
  if ([0, 1, 2, 5, 6, 18, 19].indexOf(adapter.sys) !== -1) {
    html += 'Symbol rate"><label>Symbol rate:</label>';
    if (adapter.sr) html += '<strong>' + (adapter.sr / 1000) + ' M/s</strong>';
    else html += "Unset";
  } else {
    html += 'Bandwidth"><label>Bandwidth:</label>';
    if (adapter.bw) html += '<strong>' + (adapter.bw / 1000) + ' MHz</strong>';
  }
  html += '</p>';
  
  html += '<p class="diseqc" title="DiSEqC"><label>DiSEqC:</label>';
  if (adapter.diseqc) html += '<strong>' + adapter.diseqc + '</strong>';
  html += '</p>';

  html += '</div></div>';

  // STRENGTH BAR
  
  var strength = clamp(Math.round(adapter.strength / 255 * 1000) / 10);
  html += '<div class="section"><h3><label>Strength</label>';
  if (adapter.strength) html += '<strong class="side-value">' + strength + '%</strong>';
  html += '</h3><div class="ruler strength">';
  if (adapter.strength) html += '<div class="bar" style="width:' + Math.max(strength,4) + '%"></div>';
  html += '</div></div>';
  
  // SNR BAR
  
  var snr = clamp(Math.round(adapter.snr / 255 * 1000) / 10);
  html += '<div class="section"><h3><label>SNR</label>';
  if (adapter.snr) html += '<strong class="side-value">' + snr + '%</strong>';
  html += '</h3><div class="ruler snr">';
  if (adapter.snr) html += '<div class="bar" style="width:' + Math.max(snr,4) + '%"></div>';
  html += '</div></div>';
  
  // BER BAR
  
  var ber = clamp(Math.log10(adapter.ber+1) / 5 * 100);
  html += '<div class="section"><h3><label>BER</label>';
  if (adapter.ber != 0 || adapter.snr) html += '<strong class="side-value">' + adapter.ber + '</strong>';
  html += '</h3><div class="ruler ber">';
  if (adapter.ber) html += '<div class="bar" style="width:' + Math.max(ber,4) + '%"></div>';
  html += '</div></div>';
  
  // STREAMS SECTION

  var streams = [];
  for (var i = 0; i < 16; i++) {
    if (!(st_enabled[i] && st_adapter[i] == 0)) continue;
    var stream = '<div class="stream';
    if (st_play[i]) stream += ' playing';
    stream += '"><div class="source"><h3 class="host"><span class="hostname">' + escapeHtml(st_rhost[i]) + '</span>:<span class="port">' + st_rport[i] + '</span></h3><p class="user-agent">' + escapeHtml(st_useragent[i]) + '</p></div><div class="pids">';
    stream += st_pids[i].split(",").sort(function (a, b) {
      return parseInt(a) - parseInt(b);
    }).map(function (x) {
      return '<strong>' + x + '</strong>';
    }).join(", ");
    stream += '</div></div>';
    streams.push(stream);
  }
  
  html += '<div class="section"><h3><label>Streams</label></h3><div class="streams">';
  if (streams.length) html += streams.join("");
  else html += '<div class="stream placeholder"><p>No open streams.</p></div>';
  html += '</div></div>';

  // FINISH RENDERING

  html += '</div></fieldset>';
  container.innerHTML = html;

  setTimeout(refetch, 1500);
}

function refetch() {
  try {
    var xhr = new XMLHttpRequest();
    xhr.onload = inject;
    xhr.onerror = function () { location.reload(true); };
    xhr.open("GET", location.href);
    xhr.responseType = "document";
    xhr.send();
  } catch (e) {
    location.reload(true);
  }
}

function inject() {
  try {
    eval(this.responseXML.getElementById("data-script").innerHTML);
    render();
  } catch (e) {
    location.reload(true);
  }
}

render();

function escapeHtml(str) {
  var div = document.createElement('div');
  div.appendChild(document.createTextNode(str));
  return div.innerHTML;
}

function clamp(x) {
  if (x > 100) return 100;
  if (x < 0) return 0;
  return x;
}
