const char PAGE_INDEX[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="utf-8"/>
    <meta content="width=device-width, initial-scale=1.0" name="viewport"/>
    <title>EasyLogger %TITLE%</title>
    <style>
        :root {
            --navy-900: #0f172a;
            --navy-800: #1e293b;
            --slate-800: #1e293b; /* SAME AS NAVY-800 mostly */
            --slate-700: #334155;
            --slate-600: #475569;
            --slate-500: #64748b;
            --slate-400: #94a3b8;
            --slate-300: #cbd5e1;
            --slate-200: #e2e8f0;
            --slate-100: #f1f5f9;
            --slate-50:  #f8fafc;
            
            --blue-600: #2563eb;
            --blue-500: #3b82f6;
            --blue-100: #dbeafe;
            
            --green-600: #16a34a;
            --green-500: #22c55e;
            
            --red-600: #dc2626;
            --red-200: #fecaca;
            --red-50:  #fef2f2;
            
            --yellow-400: #facc15;
            
            --bg-body: var(--slate-50);
            --text-main: var(--slate-700);
            --border-color: var(--slate-200);
            
            --font-stack: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif, "Apple Color Emoji", "Segoe UI Emoji", "Segoe UI Symbol";
            --font-mono: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, "Liberation Mono", "Courier New", monospace;
        }

        /* RESET & BASE */
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body {
            font-family: var(--font-stack);
            background-color: var(--bg-body);
            color: var(--text-main);
            display: flex;
            flex-direction: column;
            min-height: 100vh;
            line-height: 1.5;
        }

        /* UTILITIES */
        .hidden { display: none !important; }
        .block { display: block; }
        .flex { display: flex; }
        .flex-col { flex-direction: column; }
        .items-center { align-items: center; }
        .justify-between { justify-content: space-between; }
        .justify-end { justify-content: flex-end; }
        .gap-1 { gap: 0.25rem; }
        .gap-2 { gap: 0.5rem; }
        .gap-3 { gap: 0.75rem; }
        .gap-4 { gap: 1rem; }
        .gap-6 { gap: 1.5rem; }
        .gap-8 { gap: 2rem; }
        .gap-12 { gap: 3rem; }
        
        .w-full { width: 100%; }
        .max-w-screen-xl { max-width: 1600px; margin-left: auto; margin-right: auto; }
        .max-w-4xl { max-width: 56rem; margin-left: auto; margin-right: auto; }
        .max-w-md { max-width: 28rem; }
        
        .h-16 { height: 4rem; }
        .h-full { height: 100%; }
        .h-8 { height: 2rem; }
        .w-px { width: 1px; }
        .flex-grow { flex-grow: 1; }
        .overflow-hidden { overflow: hidden; }
        .overflow-y-auto { overflow-y: auto; }
        
        .p-2 { padding: 0.5rem; }
        .p-2-5 { padding: 0.625rem; }
        .p-3 { padding: 0.75rem; }
        .p-4 { padding: 1rem; }
        .p-5 { padding: 1.25rem; }
        .p-6 { padding: 1.5rem; }
        .p-10 { padding: 2.5rem; }
        
        .px-2 { padding-left: 0.5rem; padding-right: 0.5rem; }
        .px-3 { padding-left: 0.75rem; padding-right: 0.75rem; }
        .px-4 { padding-left: 1rem; padding-right: 1rem; }
        .px-6 { padding-left: 1.5rem; padding-right: 1.5rem; }
        .px-8 { padding-left: 2rem; padding-right: 2rem; }
        
        .py-1 { padding-top: 0.25rem; padding-bottom: 0.25rem; }
        .py-2 { padding-top: 0.5rem; padding-bottom: 0.5rem; }
        .py-3 { padding-top: 0.75rem; padding-bottom: 0.75rem; }
        .py-4 { padding-top: 1rem; padding-bottom: 1rem; }
        
        .pt-6 { padding-top: 1.5rem; }
        .pb-2 { padding-bottom: 0.5rem; }
        .mt-1 { margin-top: 0.25rem; }
        .mt-2 { margin-top: 0.5rem; }
        .mt-4 { margin-top: 1rem; }
        .mt-6 { margin-top: 1.5rem; }
        .mt-8 { margin-top: 2rem; }
        .mt-12 { margin-top: 3rem; }
        .mb-1 { margin-bottom: 0.25rem; }
        .mb-2 { margin-bottom: 0.5rem; }
        .mb-4 { margin-bottom: 1rem; }
        .mb-6 { margin-bottom: 1.5rem; }
        .mb-8 { margin-bottom: 2rem; }
        .ml-2 { margin-left: 0.5rem; }

        .text-center { text-align: center; }
        .text-right { text-align: right; }
        
        .font-mono { font-family: var(--font-mono); }
        .font-bold { font-weight: 700; }
        .font-semibold { font-weight: 600; }
        .font-medium { font-weight: 500; }
        .font-normal { font-weight: 400; }
        .font-extrabold { font-weight: 800; }
        
        .text-xs { font-size: 0.75rem; line-height: 1rem; }
        .text-sm { font-size: 0.875rem; line-height: 1.25rem; }
        .text-base { font-size: 1rem; line-height: 1.5rem; }
        .text-lg { font-size: 1.125rem; line-height: 1.75rem; }
        .text-xl { font-size: 1.25rem; line-height: 1.75rem; }
        .text-3xl { font-size: 1.875rem; line-height: 2.25rem; }
        .text-8xl { font-size: 6rem; line-height: 1; }
        
        .uppercase { text-transform: uppercase; }
        .tracking-wider { letter-spacing: 0.05em; }
        .tracking-widest { letter-spacing: 0.1em; }
        .tracking-tight { letter-spacing: -0.025em; }
        .tracking-tighter { letter-spacing: -0.05em; }
        .italic { font-style: italic; }

        /* COLORS */
        .bg-white { background-color: white; }
        .bg-slate-50 { background-color: var(--slate-50); }
        .bg-slate-100 { background-color: var(--slate-100); }
        .bg-slate-800 { background-color: var(--slate-800); }
        .bg-slate-900 { background-color: var(--navy-900); }
        .bg-blue-500 { background-color: var(--blue-500); }
        .bg-blue-600 { background-color: var(--blue-600); }
        
        .text-white { color: white; }
        .text-slate-400 { color: var(--slate-400); }
        .text-slate-500 { color: var(--slate-500); }
        .text-slate-600 { color: var(--slate-600); }
        .text-slate-700 { color: var(--slate-700); }
        .text-slate-800 { color: var(--slate-800); }
        .text-slate-900 { color: var(--navy-900); }
        .text-blue-500 { color: var(--blue-500); }
        .text-blue-600 { color: var(--blue-600); }
        .text-green-500 { color: var(--green-500); }
        .text-green-600 { color: var(--green-600); }
        .text-red-600 { color: var(--red-600); }
        .text-yellow-400 { color: var(--yellow-400); }

        .border { border-width: 1px; border-style: solid; }
        .border-t { border-top-width: 1px; border-top-style: solid; }
        .border-b { border-bottom-width: 1px; border-bottom-style: solid; }
        .border-slate-100 { border-color: var(--slate-100); }
        .border-slate-200 { border-color: var(--slate-200); }
        .border-slate-300 { border-color: var(--slate-300); }
        .border-blue-100 { border-color: var(--blue-100); }
        .border-blue-500 { border-color: var(--blue-500); }
        .border-red-200 { border-color: var(--red-200); }
        .border-white-10 { border-color: rgba(255,255,255,0.1); }
        
        .rounded-sm { border-radius: 0.125rem; }
        .rounded { border-radius: 0.25rem; }
        .rounded-lg { border-radius: 0.5rem; }
        .rounded-full { border-radius: 9999px; }
        
        .shadow-sm { box-shadow: 0 1px 2px 0 rgba(0, 0, 0, 0.05); }
        .shadow-lg { box-shadow: 0 10px 15px -3px rgba(0, 0, 0, 0.1), 0 4px 6px -2px rgba(0, 0, 0, 0.05); }

        /* LAYOUT GRID RESPONSIVE */
        .grid { display: grid; }
        .grid-cols-1 { grid-template-columns: repeat(1, minmax(0, 1fr)); }
        .grid-cols-2 { grid-template-columns: repeat(2, minmax(0, 1fr)); }
        .grid-cols-12 { grid-template-columns: repeat(1, minmax(0, 1fr)); } /* Mobile Default */
        
        @media (min-width: 768px) {
            .md-flex { display: flex; }
            .md-hidden { display: none; }
            .md-block { display: block; }
            .text-9xl { font-size: 8rem; line-height: 1; }
        }
        
        @media (min-width: 1024px) {
            .lg-flex { display: flex; }
            .lg-hidden { display: none; }
            .grid-cols-12 { grid-template-columns: repeat(12, minmax(0, 1fr)); }
            .col-span-3 { grid-column: span 3 / span 3; }
            .col-span-6 { grid-column: span 6 / span 6; }
            .lg-grid-cols-2 { grid-template-columns: repeat(2, minmax(0, 1fr)); }
        }

        /* CUSTOM COMPONENTS */
        .industrial-card {
            background: white;
            border: 1px solid var(--border-color);
            box-shadow: 0 1px 3px rgba(0,0,0,0.1);
            position: relative;
        }
        
        .nav-item {
            cursor: pointer;
            border-bottom: 3px solid transparent;
            color: var(--slate-500);
            transition: all 0.2s;
        }
        .nav-item:hover {
            color: var(--navy-800);
        }
        .nav-item.active {
            border-bottom: 3px solid var(--blue-500);
            color: var(--navy-900);
            font-weight: 600;
        }

        /* TAB NAV BAR */
        .tab-nav {
            background: white;
            border-bottom: 1px solid var(--slate-200);
            padding: 0 1.5rem;
        }
        .tab-nav-inner {
            max-width: 1600px;
            margin: 0 auto;
            display: flex;
            align-items: stretch;
            gap: 0;
        }
        .tab-nav-item {
            display: inline-flex;
            align-items: center;
            padding: 0.875rem 1.25rem;
            font-size: 0.875rem;
            font-weight: 500;
            color: var(--slate-500);
            background: transparent;
            border: none;
            border-bottom: 3px solid transparent;
            cursor: pointer;
            transition: color 0.2s, border-color 0.2s, background 0.2s;
            text-transform: uppercase;
            letter-spacing: 0.05em;
        }
        .tab-nav-item:hover {
            color: var(--slate-700);
            background: var(--slate-50);
        }
        .tab-nav-item.active {
            color: var(--blue-600);
            border-bottom-color: var(--blue-600);
            background: var(--slate-50);
        }
        .tab-nav-item .tab-icon {
            width: 18px;
            height: 18px;
            margin-right: 0.5rem;
            opacity: 0.8;
        }
        @media (max-width: 1023px) {
            .tab-nav .tab-nav-inner { flex-direction: column; align-items: center; }
            .tab-nav-select {
                display: block !important;
                width: 100%;
                max-width: 280px;
                margin: 0.75rem 0;
                padding: 0.5rem 0.75rem;
                border: 1px solid var(--slate-200);
                border-radius: 0.25rem;
                font-size: 0.875rem;
                background: white;
            }
            .tab-nav-list { display: none !important; }
        }
        .tab-nav-select { display: none; }

        /* ICONS (SVG REPLACEMENT) */
        .icon {
            width: 24px;
            height: 24px;
            fill: currentColor;
            display: inline-block;
            vertical-align: middle;
        }
        .icon-sm { width: 20px; height: 20px; }
        .icon-lg { width: 32px; height: 32px; }
        .icon-xl { width: 48px; height: 48px; }

        /* SWITCH COMPONENT */
        .switch {
            position: relative;
            display: inline-block;
            width: 44px;
            height: 22px;
        }
        .switch input { opacity: 0; width: 0; height: 0; }
        .slider {
            position: absolute;
            cursor: pointer;
            top: 0; left: 0; right: 0; bottom: 0;
            background-color: var(--slate-300);
            transition: .2s;
            border-radius: 34px;
        }
        .slider:before {
            position: absolute;
            content: "";
            height: 18px; width: 18px;
            left: 2px; bottom: 2px;
            background-color: white;
            transition: .2s;
            border-radius: 50%;
        }
        input:checked + .slider { background-color: var(--blue-500); }
        input:checked + .slider:before { transform: translateX(22px); }

        /* FORMS */
        input[type="text"], input[type="password"], input[type="number"], select {
            width: 100%;
            background-color: var(--slate-50); /* or white? */
            border: 1px solid var(--slate-200);
            border-radius: 0.125rem;
            padding: 0.5rem 0.75rem;
            font-size: 0.875rem;
            line-height: 1.25rem;
            outline: none;
            transition: border-color 0.15s;
        }
        input:focus, select:focus {
            border-color: var(--blue-500);
            box-shadow: 0 0 0 1px var(--blue-500);
            outline: none;
        }
        
        /* BUTTONS */
        button { cursor: pointer; border: none; }
        button:hover { opacity: 0.9; }
        button:active { transform: translateY(1px); }

        /* TABS */
        .tab-content { display: none; }
        .tab-content.active { display: block; animation: fadeIn 0.3s; }
        @keyframes fadeIn { from { opacity: 0; } to { opacity: 1; } }
        
        /* SPECIFIC OVERRIDES */
        .border-t-thick-blue { border-top: 6px solid var(--blue-600); }
        .bg-white-5 { background-color: rgba(255,255,255,0.05); }
        .border-white-10 { border-color: rgba(255,255,255,0.10); }

        /* TOAST */
        .toast-container {
            position: fixed;
            bottom: 1.5rem;
            left: 50%;
            transform: translateX(-50%);
            z-index: 9999;
            display: flex;
            flex-direction: column;
            gap: 0.5rem;
            pointer-events: none;
        }
        .toast {
            padding: 0.75rem 1.25rem;
            border-radius: 0.5rem;
            font-size: 0.875rem;
            font-weight: 500;
            color: white;
            background-color: var(--slate-800);
            box-shadow: 0 10px 15px -3px rgba(0,0,0,0.1), 0 4px 6px -2px rgba(0,0,0,0.05);
            animation: toastIn 0.3s ease-out;
            max-width: 90vw;
            text-align: center;
        }
        .toast--success { background-color: var(--green-600); }
        .toast--warning { background-color: var(--yellow-400); color: var(--navy-900); }
        @keyframes toastIn {
            from { opacity: 0; transform: translateY(10px); }
            to { opacity: 1; transform: translateY(0); }
        }

    </style>
</head>
<body>

<header class="bg-white border-b border-slate-200" style="position: sticky; top: 0; z-index: 51;">
    <div class="max-w-screen-xl px-6 h-14 flex items-center justify-between">
        <div class="flex items-center gap-2">
            <svg class="icon-lg text-blue-600" viewBox="0 0 24 24"><path d="M12 12c2.21 0 4-1.79 4-4s-1.79-4-4-4-4 1.79-4 4 1.79 4 4 4zm0 2c-2.67 0-8 1.34-8 4v2h16v-2c0-2.66-5.33-4-8-4z"/></svg>
            <span class="text-lg font-extrabold tracking-tight text-slate-900">EasyLogger</span>
            <span class="text-sm font-normal text-slate-500 bg-slate-100 px-2 py-0.5 rounded">%TITLE%</span>
        </div>
        <div class="flex items-center gap-4">
            <div class="hidden sm:flex flex-col text-right">
                <span class="text-xs uppercase font-bold text-slate-400 tracking-tighter" style="font-size: 10px;">Réseau</span>
                <span class="text-sm font-medium text-slate-700">%SSID%</span>
            </div>
            <div class="h-6 w-px bg-slate-200 hidden sm:block"></div>
            <div class="flex items-center gap-2">
                <svg id="status-icon" class="icon-sm text-green-500" viewBox="0 0 24 24"><path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm-2 15l-5-5 1.41-1.41L10 14.17l7.59-7.59L19 8l-9 9z"/></svg>
                <span class="text-xs font-bold text-slate-600 uppercase" id="status-text">Prêt</span>
            </div>
        </div>
    </div>
    <nav class="tab-nav" role="tablist" aria-label="Navigation principale">
        <div class="tab-nav-inner">
            <select class="tab-nav-select" onchange="switchTab(this.value)" aria-label="Choisir un onglet">
                <option value="monitor">Monitoring</option>
                <option value="config">Paramètres</option>
                <option value="dac">DAC2 / Diagnostics</option>
            </select>
            <div class="tab-nav-list flex">
                <button type="button" class="tab-nav-item active" role="tab" id="nav-monitor" aria-selected="true" onclick="switchTab('monitor')">
                    <svg class="tab-icon" viewBox="0 0 24 24" fill="currentColor"><path d="M19 3H5c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h14c1.1 0 2-.9 2-2V5c0-1.1-.9-2-2-2zm-5 14H7v-2h7v2zm3-4H7v-2h10v2zm0-4H7V7h10v2z"/></svg>
                    Monitoring
                </button>
                <button type="button" class="tab-nav-item" role="tab" id="nav-config" aria-selected="false" onclick="switchTab('config')">
                    <svg class="tab-icon" viewBox="0 0 24 24" fill="currentColor"><path d="M19.14 12.94c.04-.3.06-.61.06-.94 0-.32-.02-.64-.07-.94l2.03-1.58c.18-.14.23-.41.12-.61l-1.92-3.32c-.12-.22-.37-.29-.59-.22l-2.39.96c-.5-.38-1.03-.7-1.62-.94l-.36-2.54c-.04-.24-.24-.41-.48-.41h-3.84c-.24 0-.43.17-.47.41l-.36 2.54c-.59.24-1.13.57-1.62.94l-2.39-.96c-.22-.08-.47 0-.59.22L3.16 8.87c-.12.21-.08.47.12.61l2.03 1.58c-.05.3-.09.63-.09.94s.04.64.09.94l-2.03 1.58c-.18.14-.23.41-.12.61l1.92 3.32c.12.22.37.29.59.22l2.39-.96c.5.38 1.03.7 1.62.94l.36 2.54c.05.24.24.41.48.41h3.84c.24 0 .44-.17.47-.41l.36-2.54c.59-.24 1.13-.56 1.62-.94l2.39.96c.22.08.47 0 .59-.22l1.92-3.32c.12-.22.07-.47-.12-.61l-2.01-1.58zM12 15.6c-1.98 0-3.6-1.62-3.6-3.6s1.62-3.6 3.6-3.6 3.6 1.62 3.6 3.6-1.62 3.6-3.6 3.6z"/></svg>
                    Paramètres
                </button>
                <button type="button" class="tab-nav-item" role="tab" id="nav-dac" aria-selected="false" onclick="switchTab('dac')">
                    <svg class="tab-icon" viewBox="0 0 24 24" fill="currentColor"><path d="M19.5 9.5c-1.03 0-1.9.62-2.29 1.5h-2.92c-.39-.88-1.26-1.5-2.29-1.5s-1.9.62-2.29 1.5H6.79c-.39-.88-1.26-1.5-2.29-1.5C3.12 9.5 2 10.62 2 12s1.12 2.5 2.5 2.5c1.03 0 1.9-.62 2.29-1.5h2.92c.39.88 1.26 1.5 2.29 1.5s1.9-.62 2.29-1.5h2.92c.39.88 1.26 1.5 2.29 1.5 1.38 0 2.5-1.12 2.5-2.5s-1.12-2.5-2.5-2.5z"/></svg>
                    DAC2 / Diagnostics
                </button>
            </div>
        </div>
    </nav>
</header>

<div id="toast-container" class="toast-container" aria-live="polite"></div>

<main class="max-w-screen-xl p-6 w-full flex-grow">
    <form method="POST" action="/save" id="config-form" onsubmit="return submitConfigForm(event)">
    
        <!-- --- TAB: MONITORING --- -->
        <div id="tab-monitor" class="tab-content active">
            <div class="grid grid-cols-12 gap-6">
                <!-- Left Sidebar -->
                <aside class="col-span-12 col-span-3">
                    <section class="industrial-card p-5 rounded-sm mb-6">
                        <div class="flex items-center justify-between mb-6">
                            <h2 class="text-xs font-bold uppercase tracking-widest text-slate-500 flex items-center gap-2">
                                <!-- Icon: memory -->
                                <svg class="icon-sm" viewBox="0 0 24 24"><path d="M6 2h12v6l-4 4-4-4V2zM2 6h4v12H2V6zm16 0h4v12h-4V6zM6 16h12v6l-4-4-4 4v-6zM12 6c-2.21 0-4 1.79-4 4s1.79 4 4 4 4-1.79 4-4-1.79-4-4-4z"/></svg>
                                Hardware Profile
                            </h2>
                            <span class="text-xs bg-slate-100 px-2 py-1 rounded text-slate-600 font-mono" style="font-size: 10px;">ESP32-S3</span>
                        </div>
                        <div class="block">
                            <div class="flex justify-between items-center mb-1">
                                <span class="text-xs text-slate-500">Core Temp</span>
                                <span id="sys-temp" class="text-xs font-semibold text-slate-800">--.-°C</span>
                            </div>
                            <div class="w-full bg-slate-100 h-2 rounded-full mb-3" style="height: 0.5rem; border-radius: 9999px;">
                                <div id="sys-temp-bar" class="bg-blue-500 h-full rounded-full" style="width: 0%; height: 100%; border-radius: 9999px; transition: width 0.5s;"></div>
                            </div>

                            <div class="flex justify-between items-center mb-1">
                                <span class="text-xs text-slate-500">System Load</span>
                                <span id="sys-cpu" class="text-xs font-semibold text-slate-800">--%</span>
                            </div>
                            <div class="w-full bg-slate-100 h-2 rounded-full" style="height: 0.5rem; border-radius: 9999px;">
                                <div id="sys-cpu-bar" class="bg-green-500 h-full rounded-full" style="width: 0%; height: 100%; border-radius: 9999px; transition: width 0.5s;"></div>
                            </div>
                        </div>
                    </section>
                    
                    <section class="industrial-card p-5 rounded-sm mb-6">
                        <h2 class="text-xs font-bold uppercase tracking-widest text-slate-500 mb-4 flex items-center gap-2">
                            <svg class="icon-sm" viewBox="0 0 24 24"><path d="M1 9l2 2c4.97-4.97 13.03-4.97 18 0l2-2C16.93 2.93 7.08 2.93 1 9zm8 8l3 3 3-3c-1.65-1.66-4.34-1.66-6 0zm-4-4l2 2c2.76-2.76 7.24-2.76 10 0l2-2C15.14 9.14 8.87 9.14 5 13z"/></svg>
                            Réseau
                        </h2>
                        <div class="block text-sm text-slate-600 mb-3">
                            <span class="text-xs text-slate-400 uppercase block mb-1" style="font-size: 10px;">Mode actuel</span>
                            <span id="sidebar-network-mode" class="font-medium">%NETWORK_SUMMARY%</span>
                        </div>
                        <button type="button" onclick="switchTab('config')" class="w-full py-2 bg-slate-100 border border-slate-200 text-slate-700 text-xs font-bold uppercase tracking-wider rounded-sm hover:bg-slate-200 transition-colors">Paramètres réseau →</button>
                    </section>
                </aside>

                <!-- Center Content -->
                <div class="col-span-12 col-span-6">
                    <section class="industrial-card rounded-sm border-t-thick-blue p-10 flex flex-col items-center mb-6">
                        <span class="text-xs font-bold uppercase tracking-widest text-slate-400 mb-4" style="letter-spacing: 0.2em;">Current Gross Weight</span>
                        <div class="flex items-baseline gap-3">
                            <span id="screen-content" class="text-8xl md:text-9xl font-extrabold tracking-tighter text-slate-900 leading-none">%WEIGHT%</span>
                            <span class="text-3xl font-bold text-blue-600">%UNIT%</span>
                        </div>
                        <div class="mt-4 text-sm text-slate-400 font-mono" id="screen-mode">Mode: idle</div>
                        
                        <div class="mt-12 flex gap-4 w-full max-w-md">
                            <button type="button" onclick="sendCommand('T')" class="flex-grow py-4 bg-blue-600 text-white font-bold uppercase tracking-widest text-sm rounded-sm shadow-sm transition-all hover:bg-blue-500">TARE</button>
                            <button type="button" onclick="sendCommand('Z')" class="flex-grow py-4 bg-white border border-slate-300 text-slate-700 font-bold uppercase tracking-widest text-sm rounded-sm shadow-sm transition-all hover:bg-slate-50">ZERO</button>
                        </div>
                    </section>
                    
                    <section class="industrial-card rounded-sm flex flex-col overflow-hidden" style="height: 380px;">
                        <div class="bg-slate-50 border-b border-slate-200 px-4 py-3 flex items-center justify-between">
                            <div class="flex items-center gap-3">
                                <!-- Icon: terminal -->
                                <svg class="icon text-slate-400" viewBox="0 0 24 24"><path d="M20 4H4c-1.1 0-1.99.9-1.99 2L2 18c0 1.1.9 2 2 2h16c1.1 0 2-.9 2-2V6c0-1.1-.9-2-2-2zm-8 12.42l-4-4 1.41-1.41L12 13.59V8h2v8.42zM15.42 16l-1.41-1.41 4-4-1.41-1.41L12.59 13H17v3z"/></svg>
                                <span class="text-xs font-bold uppercase tracking-widest text-slate-600">System Log — COM3</span>
                            </div>
                            <div class="flex gap-4">
                                <span class="text-xs font-mono text-green-600 font-bold" style="font-size: 10px;">LIVE</span>
                            </div>
                        </div>
                        <div id="terminal" class="flex-grow p-4 font-mono text-xs text-slate-600 overflow-y-auto" style="background-color: #f1f5f9;">
                            <p class="text-slate-500">Connecting...</p>
                        </div>
                    </section>
                </div>

                <!-- Right Sidebar -->
                <aside class="col-span-12 col-span-3">
                     <section class="industrial-card p-5 rounded-sm bg-slate-900 text-white mb-6">
                        <h2 class="text-xs font-bold uppercase tracking-widest text-slate-400 mb-4">Real-time Performance</h2>
                        <div class="grid grid-cols-2 gap-4">
                            <div class="p-3 bg-white-5 rounded-sm border border-white-10">
                                <span class="text-xs text-slate-400 uppercase block mb-1" style="font-size: 10px;">State</span>
                                <span class="text-lg font-mono font-bold">ONLINE</span>
                            </div>
                            <div class="p-3 bg-white-5 rounded-sm border border-white-10">
                                <span class="text-xs text-slate-400 uppercase block mb-1" style="font-size: 10px;">Uptime</span>
                                <span class="text-lg font-mono font-bold">--:--</span>
                            </div>
                        </div>
                    </section>
                    
                    <section class="industrial-card p-5 rounded-sm mb-6">
                        <h2 class="text-xs font-bold uppercase tracking-widest text-slate-500 mb-6 flex items-center gap-2">
                             <!-- Icon: terminal/command -->
                             <svg class="icon-sm" viewBox="0 0 24 24"><path d="M2.01 21L23 12 2.01 3 2 10l15 2-15 2z"/></svg>
                             Command
                        </h2>
                        <div class="block">
                            <div class="flex gap-2 mb-2">
                                <input type="text" id="command-input" placeholder="CMD" class="flex-grow border-slate-200 rounded-sm text-sm p-2 font-mono" onkeypress="if(event.key==='Enter'){event.preventDefault(); sendCommand(this.value);}">
                                <button type="button" onclick="sendCommand(document.getElementById('command-input').value)" class="px-3 bg-slate-800 text-white text-xs font-bold rounded-sm">></button>
                            </div>
                            <button type="button" onclick="sendCommand('P')" class="w-full py-2 bg-white border border-slate-200 text-slate-500 text-xs font-bold uppercase tracking-widest hover:bg-slate-50 transition-colors rounded-sm mb-2">Print</button>
                            <button type="button" onclick="clearTerminal()" class="w-full py-2 bg-white border border-red-200 text-red-600 text-xs font-bold uppercase tracking-widest hover:bg-red-50 transition-colors rounded-sm">Clear Log</button>
                        </div>
                    </section>
                </aside>
            </div>
        </div>

        <!-- --- TAB: PARAMÈTRES --- -->
        <div id="tab-config" class="tab-content max-w-4xl mx-auto">
            <h1 class="text-xl font-bold text-slate-800 mb-6 border-b border-slate-200 pb-3">Paramètres</h1>
            
            <!-- Section: Réseau -->
            <section class="industrial-card p-6 rounded-sm mb-6">
                <h2 class="text-sm font-bold uppercase tracking-widest text-slate-500 mb-6 flex items-center gap-2">
                    <svg class="icon" viewBox="0 0 24 24"><path d="M1 9l2 2c4.97-4.97 13.03-4.97 18 0l2-2C16.93 2.93 7.08 2.93 1 9zm8 8l3 3 3-3c-1.65-1.66-4.34-1.66-6 0zm-4-4l2 2c2.76-2.76 7.24-2.76 10 0l2-2C15.14 9.14 8.87 9.14 5 13z"/></svg>
                    Réseau
                </h2>
                <div class="block">
                    <div class="flex items-center justify-between mb-4">
                        <span class="text-sm font-medium text-slate-700">Mode client Wi‑Fi (STA)</span>
                        <label class="switch">
                            <input type="checkbox" name="cli" %CLI_CHECK%>
                            <span class="slider"></span>
                        </label>
                    </div>
                    <div class="grid grid-cols-1 lg-grid-cols-2 gap-4 mb-4">
                        <div>
                            <label class="block text-xs uppercase font-bold text-slate-400 mb-1" style="font-size: 10px;">SSID</label>
                            <input class="font-mono bg-slate-50" type="text" name="ssid" value="%SSID%" placeholder="Nom du réseau"/>
                        </div>
                        <div>
                            <label class="block text-xs uppercase font-bold text-slate-400 mb-1" style="font-size: 10px;">Mot de passe</label>
                            <input class="font-mono bg-slate-50" type="password" name="pass" value="%PASS%" placeholder="********"/>
                        </div>
                    </div>
                    <div class="flex items-center justify-between p-3 bg-slate-50 rounded border border-slate-100 mb-4">
                        <span class="text-sm font-medium text-slate-700">IP en DHCP</span>
                        <label class="switch">
                            <input type="checkbox" name="ip_dhcp" %IP_DHCP_CHECK% id="ip_dhcp" onchange="toggleStaticIP()">
                            <span class="slider"></span>
                        </label>
                    </div>
                    <div id="static-ip-fields" class="grid grid-cols-2 gap-4 mb-2">
                        <div>
                            <label class="block text-xs uppercase font-bold text-slate-400 mb-1" style="font-size: 10px;">IP</label>
                            <input type="text" name="ip" value="%IP%" placeholder="192.168.1.100" class="font-mono"/>
                        </div>
                        <div>
                            <label class="block text-xs uppercase font-bold text-slate-400 mb-1" style="font-size: 10px;">Masque</label>
                            <input type="text" name="mask" value="%MASK%" placeholder="255.255.255.0" class="font-mono"/>
                        </div>
                        <div>
                            <label class="block text-xs uppercase font-bold text-slate-400 mb-1" style="font-size: 10px;">Passerelle</label>
                            <input type="text" name="gw" value="%GW%" placeholder="192.168.1.1" class="font-mono"/>
                        </div>
                        <div>
                            <label class="block text-xs uppercase font-bold text-slate-400 mb-1" style="font-size: 10px;">DNS</label>
                            <input type="text" name="dns" value="%DNS%" placeholder="8.8.8.8" class="font-mono"/>
                        </div>
                    </div>
                </div>
            </section>

            <!-- Section: Affichage -->
            <section class="industrial-card p-6 rounded-sm mb-6">
                <h2 class="text-sm font-bold uppercase tracking-widest text-slate-500 mb-6 flex items-center gap-2">
                    <svg class="icon" viewBox="0 0 24 24"><path d="M20 2H4c-1.1 0-2 .9-2 2v12c0 1.1.9 2 2 2h14l4 4V4c0-1.1-.9-2-2-2zm-2 12H6v-2h12v2zm0-3H6V9h12v2zm0-3H6V6h12v2z"/></svg>
                    Affichage
                </h2>
                <div class="block">
                    <div class="grid grid-cols-1 lg-grid-cols-2 gap-4 mb-4">
                        <div>
                            <label class="block text-xs uppercase font-bold text-slate-400 mb-1" style="font-size: 10px;">Titre appareil</label>
                            <input name="title" type="text" value="%TITLE%" maxlength="16" placeholder="Easylogger"/>
                        </div>
                        <div>
                            <label class="block text-xs uppercase font-bold text-slate-400 mb-1" style="font-size: 10px;">Texte au repos</label>
                            <input name="idle" type="text" value="%IDLE%" maxlength="16" placeholder="En attente"/>
                        </div>
                    </div>
                    <div class="grid grid-cols-2 gap-4 mb-4">
                        <div>
                            <label class="block text-xs uppercase font-bold text-slate-400 mb-1" style="font-size: 10px;">Taille police</label>
                            <input name="font" type="number" min="1" max="3" value="%FONT%">
                        </div>
                        <div>
                            <label class="block text-xs uppercase font-bold text-slate-400 mb-1" style="font-size: 10px;">Durée pesée (ms)</label>
                            <input name="showms" type="number" min="300" max="10000" value="%SHOWMS%">
                        </div>
                    </div>
                    <div class="mb-4">
                        <label class="block text-xs uppercase font-bold text-slate-400 mb-1" style="font-size: 10px;">Unité affichée</label>
                        <input name="unit" type="text" value="%UNIT%" maxlength="8" placeholder="KG, LBS...">
                    </div>
                    <div class="mb-4">
                        <label class="block text-xs uppercase font-bold text-slate-400 mb-1" style="font-size: 10px;">Format date / heure</label>
                        <select name="dtfmt" class="bg-slate-50">
                            <option value="0" %DTFMT0%>YYYY-MM-DD HH:MM:SS</option>
                            <option value="1" %DTFMT1%>DD/MM/YYYY HH:MM:SS</option>
                            <option value="2" %DTFMT2%>HH:MM:SS</option>
                            <option value="3" %DTFMT3%>HH:MM</option>
                            <option value="4" %DTFMT4%>YYYY-MM-DD</option>
                            <option value="5" %DTFMT5%>HH:MM:SS YYYY-MM-DD</option>
                            <option value="6" %DTFMT6%>HH:MM:SS DD/MM/YYYY</option>
                            <option value="7" %DTFMT7%>HH:MM YYYY-MM-DD</option>
                        </select>
                    </div>
                    <div class="flex items-center justify-between p-3 bg-slate-50 rounded border border-slate-100">
                        <span class="text-sm font-medium text-slate-700">Indicateur Wi‑Fi sur l'écran</span>
                        <label class="switch">
                            <input type="checkbox" name="wifiind" %WIFIIND_CHECK%>
                            <span class="slider"></span>
                        </label>
                    </div>
                </div>
            </section>

            <!-- Section: Interface série -->
            <section class="industrial-card p-6 rounded-sm mb-6">
                <h2 class="text-sm font-bold uppercase tracking-widest text-slate-500 mb-6 flex items-center gap-2">
                    <svg class="icon" viewBox="0 0 24 24"><path d="M20 7c-1.1 0-2 .9-2 2v6c0 1.1-.9 2-2 2h-4v-2h4v-6c0-2.21-1.79-4-4-4s-4 1.79-4 4v6c0 1.1-.9 2-2 2H4v-2h2v-6c0-2.21 1.79-4 4-4s4 1.79 4 4v6c0 1.1.9 2 2 2h2c2.21 0 4-1.79 4-4V9c0-1.1.9-2 2-2h2V5h-2v2z"/></svg>
                    Interface série (balance)
                </h2>
                    <div class="block">
                         <div class="mb-4">
                            <label class="block text-xs uppercase font-bold text-slate-400 mb-1" style="font-size: 10px;">Balance Type</label>
                            <select name="btype" id="btype" onchange="updateBalanceParams()" class="bg-slate-50">
                                <option value="0" %BTYPE0%>A&amp;D (Standard)</option>
                                <option value="1" %BTYPE1%>Sartorius (Industrial)</option>
                            </select>
                        </div>
                        <div class="grid grid-cols-2 gap-4 mb-4">
                            <div>
                                <label class="block text-xs uppercase font-bold text-slate-400 mb-1" style="font-size: 10px;">Baud Rate</label>
                                <select name="bbaud" id="bbaud">
                                    <option value="2400" %BBAUD2400%>2400</option>
                                    <option value="4800" %BBAUD4800%>4800</option>
                                    <option value="9600" %BBAUD9600%>9600</option>
                                    <option value="19200" %BBAUD19200%>19200</option>
                                </select>
                            </div>
                            <div>
                                 <label class="block text-xs uppercase font-bold text-slate-400 mb-1" style="font-size: 10px;">Parity</label>
                                 <select name="parity" id="parity">
                                    <option value="0" %PARITY0%>None</option>
                                    <option value="1" %PARITY1%>Even</option>
                                    <option value="2" %PARITY2%>Odd</option>
                                </select>
                            </div>
                        </div>
                         <div class="grid grid-cols-2 gap-4 mb-4">
                                <div>
                                    <label class="block text-xs uppercase font-bold text-slate-400 mb-1" style="font-size: 10px;">Data Bits</label>
                                    <select name="dbits" id="dbits">
                                        <option value="5" %DBITS5%>5</option>
                                        <option value="6" %DBITS6%>6</option>
                                        <option value="7" %DBITS7%>7</option>
                                        <option value="8" %DBITS8%>8</option>
                                    </select>
                                </div>
                                <div>
                                    <label class="block text-xs uppercase font-bold text-slate-400 mb-1" style="font-size: 10px;">Stop Bits</label>
                                    <select name="stopbits" id="stopbits">
                                        <option value="1" %STOPBITS1%>1</option>
                                        <option value="2" %STOPBITS2%>2</option>
                                    </select>
                                </div>
                            </div>
                            <div class="grid grid-cols-2 gap-4 mb-4">
                                <div>
                                    <label class="block text-xs uppercase font-bold text-slate-400 mb-1" style="font-size: 10px;">Handshake</label>
                                    <select name="handshake" id="handshake">
                                        <option value="0" %HANDSHAKE0%>None</option>
                                        <option value="1" %HANDSHAKE1%>RTS/CTS</option>
                                        <option value="2" %HANDSHAKE2%>XON/XOFF</option>
                                    </select>
                                </div>
                                <div>
                                    <label class="block text-xs uppercase font-bold text-slate-400 mb-1" style="font-size: 10px;">Terminator</label>
                                    <select name="terminator" id="terminator">
                                        <option value="CRLF" %TERM_CRLF%>CRLF</option>
                                        <option value="CR" %TERM_CR%>CR</option>
                                        <option value="LF" %TERM_LF%>LF</option>
                                        <option value="NONE" %TERM_NONE%>None</option>
                                    </select>
                                </div>
                            </div>
                             <div class="flex items-center justify-between p-3 bg-slate-50 rounded border border-slate-100 mt-2">
                                <span class="text-xs font-medium text-slate-700">Swap RX/TX Pins</span>
                                <label class="switch" style="transform: scale(0.75); transform-origin: right;">
                                    <input type="checkbox" name="swaprt" %SWAPRT%>
                                    <span class="slider"></span>
                                </label>
                            </div>
                    </div>
                </section>
            </div>
             <div class="pt-6 border-t border-slate-100 mt-6">
                <button type="submit" class="w-full px-8 py-3 bg-slate-800 text-white text-sm font-bold uppercase tracking-widest hover:bg-slate-700 transition-colors rounded-sm shadow-lg">Enregistrer la configuration</button>
            </div>
        </div>
        
        <!-- --- TAB: DAC/DIAG --- -->
        <div id="tab-dac" class="tab-content max-w-4xl mx-auto">
            <!-- Contrôle en temps réel (poids + sortie DAC) -->
            <section class="industrial-card p-6 rounded-sm mb-6 border-t-thick-blue">
                <h2 class="text-sm font-bold uppercase tracking-widest text-slate-500 mb-4 flex items-center gap-2">
                    <svg class="icon" viewBox="0 0 24 24" fill="currentColor"><path d="M12 8v4l3 3m6-3a9 9 0 11-18 0 9 9 0 0118 0z"/></svg>
                    Contrôle en temps réel
                </h2>
                <p class="text-xs text-slate-500 mb-4">Vérifiez le poids lu et la sortie DAC pour étalonner.</p>
                <div class="grid grid-cols-2 gap-6">
                    <div class="p-4 bg-slate-50 rounded border border-slate-200">
                        <span class="text-xs uppercase font-bold text-slate-400 block mb-1" style="font-size: 10px;">Poids actuel (balance)</span>
                        <div class="flex items-baseline gap-2">
                            <span id="dac-current-weight" class="text-2xl font-bold text-slate-800 font-mono">---</span>
                            <span class="text-sm text-slate-500">%UNIT%</span>
                        </div>
                    </div>
                    <div class="p-4 bg-slate-50 rounded border border-slate-200">
                        <span class="text-xs uppercase font-bold text-slate-400 block mb-1" style="font-size: 10px;">Sortie DAC</span>
                        <div class="flex items-baseline gap-2">
                            <span id="dac-current-mv" class="text-2xl font-bold text-blue-600 font-mono">---</span>
                            <span class="text-sm text-slate-500">mV</span>
                        </div>
                    </div>
                </div>
            </section>

             <section class="industrial-card p-6 rounded-sm mb-6">
                <div class="flex items-center justify-between mb-6">
                    <h2 class="text-sm font-bold uppercase tracking-widest text-slate-500 flex items-center gap-2">
                        <!-- Icon: linear_scale -->
                        <svg class="icon" viewBox="0 0 24 24"><path d="M19.5 9.5c-1.03 0-1.9.62-2.29 1.5h-2.92c-.39-.88-1.26-1.5-2.29-1.5s-1.9.62-2.29 1.5H6.79c-.39-.88-1.26-1.5-2.29-1.5C3.12 9.5 2 10.62 2 12s1.12 2.5 2.5 2.5c1.03 0 1.9-.62 2.29-1.5h2.92c.39.88 1.26 1.5 2.29 1.5s1.9-.62 2.29-1.5h2.92c.39.88 1.26 1.5 2.29 1.5 1.38 0 2.5-1.12 2.5-2.5s-1.12-2.5-2.5-2.5z"/></svg> 
                        DAC2 Analog Output
                    </h2>
                    <label class="switch">
                        <input type="checkbox" name="dac2en" %DAC2EN_CHECK%>
                        <span class="slider"></span>
                    </label>
                </div>
                 <div class="grid grid-cols-2 gap-6 mb-8">
                        <div>
                            <label class="block text-xs uppercase font-bold text-slate-400 mb-1" style="font-size: 10px;">Range Min (g)</label>
                            <input name="dac2min" type="number" step="0.1" value="%DAC2MIN%">
                        </div>
                        <div>
                            <label class="block text-xs uppercase font-bold text-slate-400 mb-1" style="font-size: 10px;">Range Max (g)</label>
                            <input name="dac2max" type="number" step="0.1" value="%DAC2MAX%">
                        </div>
                    </div>
                    
                    <h3 class="text-xs font-bold uppercase tracking-widest text-blue-600 mb-4 border-b border-blue-100 pb-2">6-Point Calibration Table</h3>
                    <div class="bg-slate-50 rounded p-4 border border-slate-200">
                        <div class="grid grid-cols-2 gap-4 mb-2 text-xs uppercase font-bold text-slate-400 text-center" style="font-size: 10px;">
                            <div>Weight (g)</div>
                            <div>Output (mV)</div>
                        </div>
                         <div class="block">
                            <div class="grid grid-cols-2 gap-4 mb-2">
                                <input name="dac2w0" type="number" step="0.01" value="%DAC2W0%" class="text-center">
                                <input name="dac2m0" type="number" min="0" max="10000" value="%DAC2M0%" class="text-center font-mono text-blue-600">
                            </div>
                            <div class="grid grid-cols-2 gap-4 mb-2">
                                <input name="dac2w1" type="number" step="0.01" value="%DAC2W1%" class="text-center">
                                <input name="dac2m1" type="number" min="0" max="10000" value="%DAC2M1%" class="text-center font-mono text-blue-600">
                            </div>
                            <div class="grid grid-cols-2 gap-4 mb-2">
                                <input name="dac2w2" type="number" step="0.01" value="%DAC2W2%" class="text-center">
                                <input name="dac2m2" type="number" min="0" max="10000" value="%DAC2M2%" class="text-center font-mono text-blue-600">
                            </div>
                             <div class="grid grid-cols-2 gap-4 mb-2">
                                <input name="dac2w3" type="number" step="0.01" value="%DAC2W3%" class="text-center">
                                <input name="dac2m3" type="number" min="0" max="10000" value="%DAC2M3%" class="text-center font-mono text-blue-600">
                            </div>
                             <div class="grid grid-cols-2 gap-4 mb-2">
                                <input name="dac2w4" type="number" step="0.01" value="%DAC2W4%" class="text-center">
                                <input name="dac2m4" type="number" min="0" max="10000" value="%DAC2M4%" class="text-center font-mono text-blue-600">
                            </div>
                             <div class="grid grid-cols-2 gap-4 mb-2">
                                <input name="dac2w5" type="number" step="0.01" value="%DAC2W5%" class="text-center">
                                <input name="dac2m5" type="number" min="0" max="10000" value="%DAC2M5%" class="text-center font-mono text-blue-600">
                            </div>
                        </div>
                    </div>
                     <div class="mt-6">
                        <button type="submit" class="w-full py-3 bg-slate-800 text-white text-sm font-bold uppercase tracking-widest hover:bg-slate-700 transition-colors rounded-sm shadow-lg">Save DAC Calibration</button>
                    </div>
             </section>
        </div>
        
    </form>
</main>

<footer class="mt-8 border-t border-slate-200 bg-white">
    <div class="max-w-screen-xl mx-auto px-6 py-6 flex lg-flex md-block justify-between items-center gap-8">
        <div class="flex gap-12">
            <div class="flex flex-col">
                <span class="text-xs text-slate-400 uppercase font-bold tracking-widest" style="font-size: 10px;">Process Records</span>
                <span class="text-xl font-bold text-slate-800">12,492</span>
            </div>
            <div class="flex flex-col">
                <span class="text-xs text-slate-400 uppercase font-bold tracking-widest" style="font-size: 10px;">Firmware</span>
                <span class="text-xl font-bold text-slate-800 text-blue-600">v2.4.1</span>
            </div>
        </div>
        <div class="text-right">
            <span class="text-xs text-slate-400 font-medium italic">Powered by EasyLogger</span>
        </div>
    </div>
</footer>

<script>
    function switchTab(tabId) {
        document.querySelectorAll('.tab-nav-item').forEach(function(t) {
            t.classList.remove('active');
            t.setAttribute('aria-selected', 'false');
        });
        var navEl = document.getElementById('nav-' + tabId);
        if (navEl) {
            navEl.classList.add('active');
            navEl.setAttribute('aria-selected', 'true');
        }
        document.querySelectorAll('.tab-content').forEach(function(c) { c.classList.remove('active'); });
        var contentEl = document.getElementById('tab-' + tabId);
        if (contentEl) contentEl.classList.add('active');
        var sel = document.querySelector('.tab-nav-select');
        if (sel) sel.value = tabId;
    }

    function toggleStaticIP() {
        var dhcp = document.getElementById('ip_dhcp');
        var el = document.getElementById('static-ip-fields');
        if (el && dhcp) el.style.display = dhcp.checked ? 'none' : 'grid';
    }
    document.addEventListener('DOMContentLoaded', function() { toggleStaticIP(); });

    function showToast(message, type) {
        type = type || 'success';
        var container = document.getElementById('toast-container');
        if (!container) return;
        var el = document.createElement('div');
        el.className = 'toast toast--' + type;
        el.textContent = message;
        container.appendChild(el);
        var duration = (message.indexOf('Redémarrage') !== -1) ? 6000 : 3500;
        setTimeout(function() {
            el.style.opacity = '0';
            el.style.transition = 'opacity 0.3s';
            setTimeout(function() { el.remove(); }, 300);
        }, duration);
    }

    function submitConfigForm(event) {
        event.preventDefault();
        var form = document.getElementById('config-form');
        if (!form) return false;
        var formData = new FormData(form);
        var body = new URLSearchParams();
        for (var p of formData) { body.append(p[0], p[1]); }
        fetch(form.action, {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: body.toString()
        })
        .then(function(res) { return res.text(); })
        .then(function(text) {
            showToast(text || 'Config enregistrée.', 'success');
        })
        .catch(function(err) {
            showToast('Erreur enregistrement.', 'warning');
        });
        return false;
    }

    const btype = document.getElementById('btype');
    function updateBalanceParams() {
        const elBbaud = document.getElementById('bbaud');
        const elDbits = document.getElementById('dbits');
        const elParity = document.getElementById('parity');
        const elStopbits = document.getElementById('stopbits');
        const elHandshake = document.getElementById('handshake');
        const elTerminator = document.getElementById('terminator');
        const elSwaprt = document.querySelector('input[name="swaprt"]');
        
        const typeVal = document.getElementById('btype').value;

        if (typeVal === '0') {
            if(elBbaud) elBbaud.value = '2400';
            if(elDbits) elDbits.value = '7';
            if(elParity) elParity.value = '1';
            if(elStopbits) elStopbits.value = '1';
            if(elHandshake) elHandshake.value = '0';
            if(elTerminator) elTerminator.value = 'CRLF';
            if (elSwaprt) elSwaprt.checked = false;
        } else if (typeVal === '1') {
            if(elBbaud) elBbaud.value = '9600';
            if(elDbits) elDbits.value = '8';
            if(elParity) elParity.value = '2';
            if(elStopbits) elStopbits.value = '1';
            if(elHandshake) elHandshake.value = '0';
            if(elTerminator) elTerminator.value = 'CRLF';
            if (elSwaprt) elSwaprt.checked = false;
        }
    }
    if(btype) btype.addEventListener('change', updateBalanceParams);

    let ws = null;
    let wsReconnectTimeout = null;

    function connectWebSocket() {
        const wsProtocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
        const wsHost = window.location.hostname;
        const wsPort = '81';
        const wsUrl = `${wsProtocol}//${wsHost}:${wsPort}`;
        
        console.log('[WS] Connecting to', wsUrl);
        try {
            ws = new WebSocket(wsUrl);
        } catch(e) { console.log('WebSocket init failed (local?)'); return; }
        
        ws.onopen = function() {
            document.getElementById('status-text').innerText = 'Connected';
            document.getElementById('status-icon').classList.replace('text-red-500', 'text-green-500');
            fetch('/api/monitor')
                .then(res => res.json())
                .then(data => {
                    if (data.screen) updateScreen(data.screen);
                    if (data.terminal) updateTerminal(data.terminal);
                    if (data.temp !== undefined) updateSensors(data.temp, data.cpu);
                    var wEl = document.getElementById('dac-current-weight');
                    var mEl = document.getElementById('dac-current-mv');
                    if (wEl && data.weight !== undefined) wEl.innerText = data.weight || '---';
                    if (mEl && data.dacMv !== undefined) mEl.innerText = String(data.dacMv);
                })
                .catch(err => console.log('Monitor fetch error', err));
        };
        
        ws.onmessage = function(event) {
            try {
                const data = JSON.parse(event.data);
                if (data.type === 'screen') {
                    const contentEl = document.getElementById('screen-content');
                    const modeEl = document.getElementById('screen-mode');
                    if (contentEl) contentEl.innerText = data.content || '---';
                    if (modeEl) modeEl.innerText = 'Mode: ' + (data.mode || 'idle');
                } else if (data.type === 'terminal') {
                    addTerminalLine(data.direction, data.data, data.timestamp);
                } else if (data.type === 'sys') {
                    updateSensors(data.temp, data.cpu);
                    var wEl = document.getElementById('dac-current-weight');
                    var mEl = document.getElementById('dac-current-mv');
                    if (wEl && data.weight !== undefined) wEl.innerText = data.weight || '---';
                    if (mEl && data.dacMv !== undefined) mEl.innerText = String(data.dacMv);
                } else if (data.weight) {
                    const contentEl = document.getElementById('screen-content');
                    if(contentEl) contentEl.innerText = data.weight;
                    var wEl = document.getElementById('dac-current-weight');
                    if (wEl) wEl.innerText = data.weight;
                    if (data.dacMv !== undefined) {
                        var mEl = document.getElementById('dac-current-mv');
                        if (mEl) mEl.innerText = String(data.dacMv);
                    }
                }
            } catch (e) { console.error('WS Parse Error', e); }
        };
        
        ws.onclose = function() {
            document.getElementById('status-text').innerText = 'Disconnected';
            document.getElementById('status-icon').classList.replace('text-green-500', 'text-red-500');
            wsReconnectTimeout = setTimeout(connectWebSocket, 3000);
        };
    }

    function addTerminalLine(dir, data, ts) {
        const term = document.getElementById('terminal');
        if(!term) return;
        const row = document.createElement('div');
        row.className = dir === 'TX' ? 'text-yellow-400' : 'text-blue-500';
        const time = ts ? ts : new Date().toLocaleTimeString();
        row.innerText = `[${time}] ${dir}: ${data}`;
        term.appendChild(row);
        term.scrollTop = term.scrollHeight;
        if(term.childElementCount > 100) term.removeChild(term.firstChild);
    }
    
    function updateTerminal(log) {
       const term = document.getElementById('terminal');
       if(!term || !log) return;
       term.innerHTML = ''; 
       const lines = log.split('\n');
       lines.forEach(l => {
           if(l) {
               const d = document.createElement('div');
               d.innerText = l;
               term.appendChild(d);
           }
       });
       term.scrollTop = term.scrollHeight;
    }

    function clearTerminal() {
        const term = document.getElementById('terminal');
        if(term) term.innerHTML = '<div class="text-slate-500">Terminal cleared.</div>';
    }

    function updateSensors(temp, cpu) {
        if(temp !== undefined) {
             const tEl = document.getElementById('sys-temp');
             const tBar = document.getElementById('sys-temp-bar');
             if(tEl) tEl.innerText = temp.toFixed(1) + '°C';
             // 0-80°C mapping
             let tp = (temp / 80.0) * 100;
             if(tp>100) tp=100;
             if(tBar) tBar.style.width = tp + '%';
        }
        if(cpu !== undefined) {
             const cEl = document.getElementById('sys-cpu');
             const cBar = document.getElementById('sys-cpu-bar');
             if(cEl) cEl.innerText = Math.round(cpu) + '%';
             if(cBar) {
                 cBar.style.width = cpu + '%';
                 if(cpu > 80) cBar.classList.replace('bg-green-500', 'bg-red-600');
                 else if(cpu > 50) cBar.classList.replace('bg-green-500', 'text-yellow-400'); // Note: bg color logic simplified
                 else cBar.style.backgroundColor = 'var(--green-500)';
             }
        }
    }

    function sendCommand(cmd) {
        if(!cmd) return;
        addTerminalLine('TX', cmd);
        fetch('/api/balance/command?cmd=' + encodeURIComponent(cmd), { method: 'POST' })
            .catch(e => console.error('Send failed', e));
        const input = document.getElementById('command-input');
        if(input) input.value = '';
    }

    window.addEventListener('load', connectWebSocket);
</script>
</body>
</html>
)HTML";
