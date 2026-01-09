import { Component, inject, signal, OnInit } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { VectorDbService } from '../services/vector-db.service';
import { AppSettings, LogLevel, EMBEDDING_MODELS } from '../models/core';

@Component({
  selector: 'app-settings',
  standalone: true,
  imports: [CommonModule, FormsModule],
  template: `
    <div class="h-full overflow-y-auto custom-scrollbar bg-[#09090b]">
      <div class="max-w-3xl mx-auto p-8 space-y-8">
        <!-- Header -->
        <div>
          <h2 class="text-2xl font-bold text-white mb-2">Settings</h2>
          <p class="text-zinc-500">Configure application preferences and connections</p>
        </div>

        <!-- API Configuration -->
        <section class="rounded-xl border border-white/5 bg-white/[0.02] overflow-hidden">
          <div class="px-6 py-4 border-b border-white/5">
            <h3 class="font-semibold text-white">API Configuration</h3>
          </div>
          <div class="p-6 space-y-6">
            <div>
              <label class="block text-xs font-bold text-zinc-500 uppercase tracking-wider mb-2">Backend URL</label>
              <input 
                type="text" 
                [(ngModel)]="settings.apiUrl"
                placeholder="http://localhost:8080"
                class="w-full bg-zinc-900/50 border border-white/10 rounded-lg px-4 py-2.5 text-white text-sm focus:outline-none focus:border-indigo-500/50 placeholder-zinc-600 font-mono"
              />
            </div>

            <div>
              <label class="block text-xs font-bold text-zinc-500 uppercase tracking-wider mb-2">
                Gemini API Key
                <span class="text-zinc-600 normal-case font-normal ml-1">(for client-side embeddings)</span>
              </label>
              <input 
                type="password" 
                [(ngModel)]="settings.apiKey"
                placeholder="AIza..."
                class="w-full bg-zinc-900/50 border border-white/10 rounded-lg px-4 py-2.5 text-white text-sm focus:outline-none focus:border-indigo-500/50 placeholder-zinc-600 font-mono"
              />
            </div>

            <div class="flex items-center gap-4">
              <button 
                (click)="testConnection()"
                [disabled]="isTesting()"
                class="px-4 py-2 bg-indigo-600 hover:bg-indigo-500 disabled:bg-zinc-800 text-white text-sm font-medium rounded-lg transition-colors"
              >
                {{ isTesting() ? 'Testing...' : 'Test Connection' }}
              </button>
              @if (connectionStatus()) {
                <div class="flex items-center gap-2">
                  @if (connectionStatus() === 'success') {
                    <svg xmlns="http://www.w3.org/2000/svg" class="h-5 w-5 text-emerald-500" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                      <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M5 13l4 4L19 7" />
                    </svg>
                    <span class="text-sm text-emerald-400">Connected</span>
                  } @else {
                    <svg xmlns="http://www.w3.org/2000/svg" class="h-5 w-5 text-rose-500" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                      <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M6 18L18 6M6 6l12 12" />
                    </svg>
                    <span class="text-sm text-rose-400">Connection failed</span>
                  }
                </div>
              }
            </div>
          </div>
        </section>

        <!-- Default Settings -->
        <section class="rounded-xl border border-white/5 bg-white/[0.02] overflow-hidden">
          <div class="px-6 py-4 border-b border-white/5">
            <h3 class="font-semibold text-white">Collection Defaults</h3>
          </div>
          <div class="p-6 space-y-6">
            <div class="grid grid-cols-1 md:grid-cols-2 gap-6">
              <div>
                <label class="block text-xs font-bold text-zinc-500 uppercase tracking-wider mb-2">Default Embedding Model</label>
                <select 
                  [(ngModel)]="settings.defaultEmbeddingModel"
                  class="w-full bg-zinc-900/50 border border-white/10 rounded-lg px-4 py-2.5 text-white text-sm focus:outline-none focus:border-indigo-500/50"
                >
                  @for (model of embeddingModels; track model.id) {
                    <option [value]="model.id">{{ model.name }} ({{ model.dim }}d)</option>
                  }
                </select>
              </div>

              <div>
                <label class="block text-xs font-bold text-zinc-500 uppercase tracking-wider mb-2">Default Dimension</label>
                <input 
                  type="number" 
                  [(ngModel)]="settings.defaultDimension"
                  min="64" 
                  max="4096"
                  class="w-full bg-zinc-900/50 border border-white/10 rounded-lg px-4 py-2.5 text-white text-sm focus:outline-none focus:border-indigo-500/50"
                />
              </div>

              <div>
                <label class="block text-xs font-bold text-zinc-500 uppercase tracking-wider mb-2">Default Distance Metric</label>
                <select 
                  [(ngModel)]="settings.defaultMetric"
                  class="w-full bg-zinc-900/50 border border-white/10 rounded-lg px-4 py-2.5 text-white text-sm focus:outline-none focus:border-indigo-500/50"
                >
                  <option value="cosine">Cosine Similarity</option>
                  <option value="euclidean">Euclidean Distance</option>
                  <option value="dot">Dot Product</option>
                </select>
              </div>

              <div>
                <label class="block text-xs font-bold text-zinc-500 uppercase tracking-wider mb-2">Log Level</label>
                <select 
                  [(ngModel)]="settings.logLevel"
                  class="w-full bg-zinc-900/50 border border-white/10 rounded-lg px-4 py-2.5 text-white text-sm focus:outline-none focus:border-indigo-500/50"
                >
                  <option value="debug">Debug</option>
                  <option value="info">Info</option>
                  <option value="warn">Warning</option>
                  <option value="error">Error Only</option>
                </select>
              </div>
            </div>
          </div>
        </section>

        <!-- Save Button -->
        <div class="flex justify-end gap-4">
          <button 
            (click)="resetToDefaults()"
            class="px-5 py-2.5 bg-white/5 hover:bg-white/10 text-zinc-300 text-sm font-medium rounded-lg transition-colors border border-white/5"
          >
            Reset to Defaults
          </button>
          <button 
            (click)="saveSettings()"
            [disabled]="isSaving()"
            class="px-5 py-2.5 bg-indigo-600 hover:bg-indigo-500 disabled:bg-zinc-800 text-white text-sm font-medium rounded-lg transition-colors"
          >
            {{ isSaving() ? 'Saving...' : 'Save Settings' }}
          </button>
        </div>

        <!-- About -->
        <section class="rounded-xl border border-white/5 bg-white/[0.02] overflow-hidden">
          <div class="px-6 py-4 border-b border-white/5">
            <h3 class="font-semibold text-white">About</h3>
          </div>
          <div class="p-6 space-y-4">
            <div class="flex items-center gap-4">
              <div class="w-12 h-12 bg-white rounded-xl shadow-md"></div>
              <div>
                <div class="text-lg font-bold text-white">HEKTOR Vector Studio</div>
                <div class="text-sm text-zinc-500">Agentic Vector Operations Console</div>
                <a href="https://artifact-virtual.gitbook.io/hektor/" class="text-amber-400 hover:underline mt-2 block">Docs</a>
              </div>
            </div>

            <div class="grid grid-cols-2 gap-4 pt-4 border-t border-white/5">
              <div>
                <div class="text-xs text-zinc-500 uppercase tracking-wider mb-1">UI Version</div>
                <div class="text-sm font-mono text-indigo-400">3.0.0</div>
              </div>
              <div>
                <div class="text-xs text-zinc-500 uppercase tracking-wider mb-1">Backend Version</div>
                <div class="text-sm font-mono text-indigo-400">{{ backendVersion() || 'Unknown' }}</div>
              </div>
              <div>
                <div class="text-xs text-zinc-500 uppercase tracking-wider mb-1">Angular</div>
                <div class="text-sm font-mono text-zinc-400">21.0</div>
              </div>
              <div>
                <div class="text-xs text-zinc-500 uppercase tracking-wider mb-1">License</div>
                <div class="text-sm font-mono text-zinc-400">MIT</div>
              </div>
            </div>

            <div class="pt-4 border-t border-white/5 text-center">
              <p class="text-xs text-zinc-600">
                © 2026 HEKTOR Project. Built with ❤️ for the AI-native future.
              </p>
            </div>
          </div>
        </section>
      </div>
    </div>
  `
})
export class SettingsComponent implements OnInit {
  private db = inject(VectorDbService);

  embeddingModels = EMBEDDING_MODELS;

  settings: AppSettings = {
    apiUrl: 'http://localhost:8080',
    apiKey: '',
    defaultEmbeddingModel: 'text-embedding-004',
    defaultDimension: 768,
    defaultMetric: 'cosine',
    logLevel: 'info'
  };

  isTesting = signal(false);
  isSaving = signal(false);
  connectionStatus = signal<'success' | 'error' | null>(null);
  backendVersion = signal<string | null>(null);

  ngOnInit() {
    this.loadSettings();
  }

  loadSettings() {
    const saved = localStorage.getItem('hektor_settings');
    if (saved) {
      try {
        this.settings = { ...this.settings, ...JSON.parse(saved) };
      } catch (e) {
        console.error('Failed to load settings:', e);
      }
    }
  }

  async testConnection() {
    this.isTesting.set(true);
    this.connectionStatus.set(null);

    try {
      const response = await fetch(`${this.settings.apiUrl}/health`);
      if (response.ok) {
        const data = await response.json();
        this.connectionStatus.set('success');
        this.backendVersion.set(data.version || 'Unknown');
      } else {
        this.connectionStatus.set('error');
      }
    } catch (error) {
      console.error('Connection test failed:', error);
      this.connectionStatus.set('error');
    } finally {
      this.isTesting.set(false);
    }
  }

  async saveSettings() {
    this.isSaving.set(true);
    try {
      localStorage.setItem('hektor_settings', JSON.stringify(this.settings));
      // Could also sync to backend if needed
      await new Promise(resolve => setTimeout(resolve, 500));
    } catch (error) {
      console.error('Failed to save settings:', error);
    } finally {
      this.isSaving.set(false);
    }
  }

  resetToDefaults() {
    this.settings = {
      apiUrl: 'http://localhost:8080',
      apiKey: '',
      defaultEmbeddingModel: 'text-embedding-004',
      defaultDimension: 768,
      defaultMetric: 'cosine',
      logLevel: 'info'
    };
  }
}
