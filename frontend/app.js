const API = 'http://localhost:8080/api';
const $ = id => document.getElementById(id);
let properties = [];
let toastTimer;

function page(id) {
  document.querySelectorAll('.page').forEach(section => section.classList.remove('active'));
  document.querySelectorAll('.nav').forEach(button => button.classList.remove('active'));
  $(id).classList.add('active');
  document.querySelector(`[data-page="${id}"]`)?.classList.add('active');
  if (id === 'properties') loadProps();
  if (id === 'enquiries') loadRequests();
  if (id === 'bookings') loadBookings();
  window.scrollTo({ top: 0, behavior: 'smooth' });
}

document.querySelectorAll('.nav').forEach(button => {
  button.addEventListener('click', () => page(button.dataset.page));
});

async function api(path, options = {}) {
  const headers = { ...options.headers };
  if (options.body && !headers['Content-Type']) headers['Content-Type'] = 'application/json';
  const response = await fetch(API + path, { ...options, headers });
  const data = await response.json();
  if (!response.ok) throw Error(data.message || 'Something went wrong. Please try again.');
  return data;
}

function toast(message) {
  $('toast').textContent = message;
  $('toast').classList.add('show');
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => $('toast').classList.remove('show'), 3000);
}

function safe(value) {
  return String(value ?? '')
    .replaceAll('&', '&amp;')
    .replaceAll('<', '&lt;')
    .replaceAll('>', '&gt;')
    .replaceAll('"', '&quot;')
    .replaceAll("'", '&#039;');
}

function money(value) {
  return new Intl.NumberFormat('en-IN', {
    style: 'currency',
    currency: 'INR',
    maximumFractionDigits: 0
  }).format(value);
}

function propertyCard(property) {
  const type = safe(property.type);
  const typeClass = String(property.type).toLowerCase().replace(/[^a-z0-9]+/g, '-');
  const availability = property.available ? 'Available' : 'Reserved';
  return `
    <article class="property-card">
      <div class="property-visual visual-${typeClass}">
        <span class="visual-type">${type}</span>
        <div class="visual-building" aria-hidden="true"><i></i><i></i><i></i></div>
        <span class="visual-availability ${property.available ? '' : 'reserved'}">${availability}</span>
      </div>
      <div class="property-body">
        <div class="property-title-row">
          <h3>${type}</h3>
          <span class="listing-code">${safe(property.id)}</span>
        </div>
        <p class="property-location">${safe(property.location)}${property.owner ? ` · Listed by ${safe(property.owner)}` : ''}</p>
        <div class="property-price">${money(property.price)} <small>asking price</small></div>
        <div class="property-meta"><span>${safe(property.area)} sq.ft</span><span>${availability}</span></div>
        <div class="property-actions">
          <button class="button" data-action="reserve" data-id="${safe(property.id)}" ${property.available ? '' : 'disabled'}>
            Reserve
          </button>
          <button class="button button-secondary" data-action="enquire" data-id="${safe(property.id)}">
            Ask a question
          </button>
        </div>
      </div>
    </article>`;
}

function renderProperties(query = '') {
  const normalized = query.trim().toLowerCase();
  const matches = properties.filter(property => {
    const text = `${property.id} ${property.type} ${property.location} ${property.owner}`.toLowerCase();
    return text.includes(normalized);
  });
  const featured = $('featuredGrid');
  const grid = $('propertyGrid');

  if (featured) {
    featured.innerHTML = properties.length
      ? properties.slice(0, 3).map(propertyCard).join('')
      : '<div class="empty-state"><strong>New homes are on their way</strong><p>Please check back soon.</p></div>';
  }
  if (grid) {
    grid.innerHTML = matches.length
      ? matches.map(propertyCard).join('')
      : '<div class="empty-state"><strong>No homes found</strong><p>Try a different city, home type, or listing reference.</p></div>';
  }

  $('totalProperties').textContent = properties.length;
  $('availableProperties').textContent = properties.filter(property => property.available).length;
}

async function loadProps() {
  try {
    properties = await api('/properties');
    renderProperties($('listingSearch').value);
  } catch (error) {
    toast(error.message);
    $('propertyGrid').innerHTML = '<div class="empty-state"><strong>Homes are temporarily unavailable</strong><p>Please try again in a moment.</p></div>';
  }
}

function openContact(intent, propertyId = '') {
  const dialog = $('contactDialog');
  const isReservation = intent === 'reserve';
  $('contactIntent').value = intent;
  $('contactPropertyId').value = propertyId;
  $('contactEyebrow').textContent = isReservation ? 'RESERVATION REQUEST' : 'PROPERTY ENQUIRY';
  $('contactTitle').textContent = isReservation ? 'Reserve this home' : 'Ask us anything';
  $('contactDescription').textContent = isReservation
    ? 'Share your details and our team will help you with the next steps.'
    : 'Leave your details and a note. Our team will get back to you soon.';
  $('contactSubmit').textContent = isReservation ? 'Confirm reservation' : 'Send message';
  $('contactForm').reset();
  $('contactIntent').value = intent;
  $('contactPropertyId').value = propertyId;
  $('contactMessage').value = isReservation ? 'I would like to reserve this property.' : '';
  $('contactPhone').required = isReservation;
  dialog.showModal();
  $('contactName').focus();
}

function closeContact() {
  $('contactDialog').close();
}

document.addEventListener('click', event => {
  const button = event.target.closest('[data-action]');
  if (!button) return;
  openContact(button.dataset.action, button.dataset.id);
});

$('contactForm').addEventListener('submit', async event => {
  event.preventDefault();
  const intent = $('contactIntent').value;
  const propertyId = $('contactPropertyId').value;
  const customer = $('contactName').value.trim();
  const phone = $('contactPhone').value.trim();
  const message = $('contactMessage').value.trim();

  try {
    if (intent === 'reserve') {
      const result = await api('/bookings', {
        method: 'POST',
        body: JSON.stringify({
          propertyId,
          customer,
          phone,
          date: new Date().toISOString().slice(0, 10)
        })
      });
      if (!result.success) throw Error(result.message || 'We could not complete this reservation.');
      toast('Your reservation is confirmed. Thank you!');
      await loadProps();
      refresh();
    } else {
      const result = await api('/requests', {
        method: 'POST',
        body: JSON.stringify({ customer, phone, propertyId, message })
      });
      if (!result.success) throw Error(result.message || 'We could not send your enquiry.');
      toast('Your enquiry has been sent. We will be in touch soon.');
      refresh();
    }
    closeContact();
  } catch (error) {
    toast(error.message);
  }
});

$('heroSearch').addEventListener('submit', event => {
  event.preventDefault();
  $('listingSearch').value = $('heroSearchInput').value;
  page('properties');
  renderProperties($('listingSearch').value);
});

$('propertySearchForm').addEventListener('submit', event => {
  event.preventDefault();
  renderProperties($('listingSearch').value);
});

$('propertySearchForm').addEventListener('reset', () => {
  setTimeout(() => renderProperties(''), 0);
});

$('propertyForm').addEventListener('submit', async event => {
  event.preventDefault();
  const form = event.currentTarget;
  const values = new FormData(form);
  const nextNumber = properties.reduce((max, property) => {
    const match = String(property.id).match(/^RE(\d+)$/i);
    return match ? Math.max(max, Number(match[1])) : max;
  }, 100) + 1;

  try {
    const result = await api('/properties', {
      method: 'POST',
      body: JSON.stringify({
        id: `RE${nextNumber}`,
        owner: values.get('owner'),
        location: values.get('location'),
        type: values.get('type'),
        price: Number(values.get('price')),
        area: Number(values.get('area')),
        available: true
      })
    });
    if (!result.success) throw Error(result.message || 'We could not add this property.');
    form.reset();
    await loadProps();
    page('properties');
    toast('Your property listing has been added.');
  } catch (error) {
    toast(error.message);
  }
});

$('requestForm').addEventListener('submit', async event => {
  event.preventDefault();
  const values = new FormData(event.currentTarget);
  try {
    const result = await api('/requests', {
      method: 'POST',
      body: JSON.stringify({
        customer: values.get('customer'),
        phone: values.get('phone'),
        propertyId: values.get('propertyId'),
        message: values.get('message')
      })
    });
    if (!result.success) throw Error(result.message || 'We could not send your message.');
    event.currentTarget.reset();
    toast('Your message has been sent. We will be in touch soon.');
    loadRequests();
    refresh();
  } catch (error) {
    toast(error.message);
  }
});

async function loadRequests() {
  try {
    const requests = await api('/requests');
    $('requestCount').textContent = requests.length;
    $('requestsTable').innerHTML = requests.length
      ? `<table><thead><tr><th>Received</th><th>Customer</th><th>Listing</th><th>Message</th><th>Status</th></tr></thead><tbody>${requests.map(request => `
        <tr><td>#${request.requestId}</td><td>${safe(request.customer)}</td><td>${safe(request.propertyId)}</td>
        <td>${safe(request.message)}</td><td><span class="status-pill">${request.status === 'Waiting' ? 'Received' : 'Reviewed'}</span></td></tr>`).join('')}</tbody></table>`
      : '<div class="empty-state"><strong>No enquiries yet</strong><p>Messages from interested customers will appear here.</p></div>';
  } catch (error) {
    toast(error.message);
  }
}

async function reviewNextRequest() {
  try {
    const result = await api('/requests/process', { method: 'POST' });
    toast(result.success ? 'The enquiry has been marked as reviewed.' : 'There are no enquiries to review.');
    loadRequests();
    refresh();
  } catch (error) {
    toast(error.message);
  }
}

async function loadBookings() {
  try {
    const bookings = await api('/bookings');
    $('bookingCount').textContent = bookings.length;
    $('bookingsTable').innerHTML = bookings.length
      ? `<table><thead><tr><th>Reservation</th><th>Listing</th><th>Customer</th><th>Phone</th><th>Date</th><th>Status</th></tr></thead><tbody>${bookings.map(booking => `
        <tr><td>#${booking.bookingId}</td><td>${safe(booking.propertyId)}</td><td>${safe(booking.customer)}</td>
        <td>${safe(booking.phone) || '—'}</td><td>${safe(booking.date)}</td><td><span class="status-pill reserved">${safe(booking.status)}</span></td></tr>`).join('')}</tbody></table>`
      : '<div class="empty-state"><strong>No reservations yet</strong><p>Confirmed reservations will appear here.</p></div>';
  } catch (error) {
    toast(error.message);
  }
}

async function cancelLatest() {
  if (!confirm('Cancel the most recent reservation?')) return;
  try {
    const result = await api('/bookings/cancel-latest', { method: 'POST' });
    toast(result.success ? 'The reservation has been cancelled.' : 'There are no reservations to cancel.');
    await loadProps();
    loadBookings();
    refresh();
  } catch (error) {
    toast(error.message);
  }
}

async function refresh() {
  try {
    const propertyList = await api('/properties');
    const requests = await api('/requests');
    const bookings = await api('/bookings');
    properties = propertyList;
    renderProperties($('listingSearch').value);
    $('requestCount').textContent = requests.length;
    $('bookingCount').textContent = bookings.length;
  } catch (error) {
    toast(error.message);
  }
}

async function health() {
  try {
    await api('/health');
    $('status').textContent = 'Here to help';
    $('status').classList.remove('offline');
  } catch {
    $('status').textContent = 'Service unavailable';
    $('status').classList.add('offline');
  }
}

async function initialize() {
  await health();
  await loadProps();
  await loadRequests();
  await loadBookings();
}

initialize();

window.page = page;
window.reviewNextRequest = reviewNextRequest;
window.cancelLatest = cancelLatest;
