import { Component, inject, signal, Input } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { VectorDbService } from '../services/vector-db.service';
import { HektorApiService } from '../services/hektor-api.service';
import { ExportFormat, TripletStrategy } from '../models/core';
import { environment } from '../environments/environment';

@Component({
    selector: 'app-export-manager',
    standalone: true,
    imports: [CommonModule, FormsModule],
    template: `
    <div class="h-full overflow-y-auto custom-scrollbar bg-[#09090b]">
      <div class="max-w-4xl mx-auto p-8 space-y-8">
        <!-- Header -->
        <div>
          <h2 class="text-2xl font-bold text-white mb-2">Export Manager</h2>
          <p class="text-zinc-500">Export data, training pairs, and triplets for ML training</p>
        </div>

        <!-- Tab Navigation -->
        <div class="flex bg-zinc-900/50 p-1 rounded-lg border border-white/5 inline-flex">
          <button 
            (click)="activeTab.set('data')"
            class="px-5 py-2 text-sm font-medium rounded-md transition-all"
            [class.bg-zinc-800]="activeTab() === 'data'"
            [class.text-white]="activeTab() === 'data'"
            [class.text-zinc-500]="activeTab() !== 'data'"
          >Export Data</button>
          <button 
            (click)="activeTab.set('pairs')"
            class="px-5 py-2 text-sm font-medium rounded-md transition-all"
            [class.bg-zinc-800]="activeTab() === 'pairs'"
            [class.text-white]="activeTab() === 'pairs'"
            [class.text-zinc-500]="activeTab() !== 'pairs'"
          >Training Pairs</button>
          <button 
            (click)="activeTab.set('triplets')"
            class="px-5 py-2 text-sm font-medium rounded-md transition-all"
            [class.bg-zinc-800]="activeTab() === 'triplets'"
            [class.text-white]="activeTab() === 'triplets'"
            [class.text-zinc-500]="activeTab() !== 'triplets'"
          >Triplets</button>
        </div>

        <!-- Export Data Tab -->
        @if (activeTab() === 'data') {
          <section class="rounded-xl border border-white/5 bg-white/[0.02] overflow-hidden">
            <div class="px-6 py-4 border-b border-white/5">
              <h3 class="font-semibold text-white">Export Collection Data</h3>
              <p class="text-sm text-zinc-500 mt-1">Download documents and vectors in various formats</p>
            </div>
            <div class="p-6 space-y-6">
              <div class="grid grid-cols-1 md:grid-cols-2 gap-6">
                <!-- Format -->
                <div>
                  <label class="block text-xs font-bold text-zinc-500 uppercase tracking-wider mb-2">Format</label>
                  <select 
                    [(ngModel)]="dataExport.format"
                    class="w-full bg-zinc-900/50 border border-white/10 rounded-lg px-4 py-2.5 text-white text-sm focus:outline-none focus:border-indigo-500/50"
                  >
                    <option value="jsonl">JSONL (Recommended)</option>
                    <option value="json">JSON</option>
                    <option value="csv">CSV</option>
                    <option value="parquet">Parquet</option>
                  </select>
                </div>

                <!-- Include Vectors -->
                <div>
                  <label class="block text-xs font-bold text-zinc-500 uppercase tracking-wider mb-2">Include Vectors</label>
                  <label class="flex items-center gap-3 cursor-pointer">
                    <input 
                      type="checkbox" 
                      [(ngModel)]="dataExport.includeVectors"
                      class="w-5 h-5 rounded bg-zinc-800 border-zinc-700 text-indigo-500 focus:ring-indigo-500 focus:ring-offset-0"
                    />
                    <span class="text-sm text-zinc-300">Include embedding vectors in export</span>
                  </label>
                </div>

                <!-- Date From -->
                <div>
                  <label class="block text-xs font-bold text-zinc-500 uppercase tracking-wider mb-2">From Date (Optional)</label>
                  <input 
                    type="date" 
                    [(ngModel)]="dataExport.dateFrom"
                    class="w-full bg-zinc-900/50 border border-white/10 rounded-lg px-4 py-2.5 text-white text-sm focus:outline-none focus:border-indigo-500/50"
                  />
                </div>

                <!-- Date To -->
                <div>
                  <label class="block text-xs font-bold text-zinc-500 uppercase tracking-wider mb-2">To Date (Optional)</label>
                  <input 
                    type="date" 
                    [(ngModel)]="dataExport.dateTo"
                    class="w-full bg-zinc-900/50 border border-white/10 rounded-lg px-4 py-2.5 text-white text-sm focus:outline-none focus:border-indigo-500/50"
                  />
                </div>
              </div>

              @if (exportProgress() !== null) {
                <div class="space-y-2">
                  <div class="flex justify-between text-xs text-zinc-400">
                    <span>Exporting...</span>
                    <span>{{ exportProgress() }}%</span>
                  </div>
                  <div class="h-2 bg-zinc-800 rounded-full overflow-hidden">
                    <div 
                      class="h-full bg-gradient-to-r from-emerald-500 to-teal-500 transition-all duration-300"
                      [style.width.%]="exportProgress()"
                    ></div>
                  </div>
                </div>
              }

              <button 
                (click)="exportData()"
                [disabled]="isExporting()"
                class="px-5 py-2.5 bg-emerald-600 hover:bg-emerald-500 disabled:bg-zinc-800 disabled:text-zinc-500 text-white text-sm font-medium rounded-lg transition-colors flex items-center gap-2"
              >
                <svg xmlns="http://www.w3.org/2000/svg" class="h-4 w-4" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                  <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M4 16v1a3 3 0 003 3h10a3 3 0 003-3v-1m-4-4l-4 4m0 0l-4-4m4 4V4" />
                </svg>
                {{ isExporting() ? 'Exporting...' : 'Export Data' }}
              </button>
            </div>
          </section>
        }

        <!-- Training Pairs Tab -->
        @if (activeTab() === 'pairs') {
          <section class="rounded-xl border border-white/5 bg-white/[0.02] overflow-hidden">
            <div class="px-6 py-4 border-b border-white/5">
              <h3 class="font-semibold text-white">Export Training Pairs</h3>
              <p class="text-sm text-zinc-500 mt-1">Generate positive pairs for contrastive learning</p>
            </div>
            <div class="p-6 space-y-6">
              <div>
                <label class="block text-xs font-bold text-zinc-500 uppercase tracking-wider mb-2">
                  Minimum Similarity Score: {{ pairsExport.minScore.toFixed(2) }}
                </label>
                <input 
                  type="range" 
                  [(ngModel)]="pairsExport.minScore"
                  min="0.5" 
                  max="0.99" 
                  step="0.01"
                  class="w-full accent-indigo-500"
                />
                <p class="text-xs text-zinc-600 mt-2">
                  Only pairs with similarity above this threshold will be included
                </p>
              </div>

              <button 
                (click)="exportPairs()"
                [disabled]="isExporting()"
                class="px-5 py-2.5 bg-indigo-600 hover:bg-indigo-500 disabled:bg-zinc-800 disabled:text-zinc-500 text-white text-sm font-medium rounded-lg transition-colors flex items-center gap-2"
              >
                <svg xmlns="http://www.w3.org/2000/svg" class="h-4 w-4" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                  <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M4 16v1a3 3 0 003 3h10a3 3 0 003-3v-1m-4-4l-4 4m0 0l-4-4m4 4V4" />
                </svg>
                {{ isExporting() ? 'Generating...' : 'Generate Training Pairs' }}
              </button>
            </div>
          </section>
        }

        <!-- Triplets Tab -->
        @if (activeTab() === 'triplets') {
          <section class="rounded-xl border border-white/5 bg-white/[0.02] overflow-hidden">
            <div class="px-6 py-4 border-b border-white/5">
              <h3 class="font-semibold text-white">Export Training Triplets</h3>
              <p class="text-sm text-zinc-500 mt-1">Generate (anchor, positive, negative) triplets for model fine-tuning</p>
            </div>
            <div class="p-6 space-y-6">
              <div class="grid grid-cols-1 md:grid-cols-2 gap-6">
                <!-- Negative Samples -->
                <div>
                  <label class="block text-xs font-bold text-zinc-500 uppercase tracking-wider mb-2">Negative Samples per Anchor</label>
                  <input 
                    type="number" 
                    [(ngModel)]="tripletsExport.negativeSamples"
                    min="1" 
                    max="50"
                    class="w-full bg-zinc-900/50 border border-white/10 rounded-lg px-4 py-2.5 text-white text-sm focus:outline-none focus:border-indigo-500/50"
                  />
                </div>

                <!-- Strategy -->
                <div>
                  <label class="block text-xs font-bold text-zinc-500 uppercase tracking-wider mb-2">Negative Mining Strategy</label>
                  <select 
                    [(ngModel)]="tripletsExport.strategy"
                    class="w-full bg-zinc-900/50 border border-white/10 rounded-lg px-4 py-2.5 text-white text-sm focus:outline-none focus:border-indigo-500/50"
                  >
                    <option value="hard">Hard Negatives (Highest Quality)</option>
                    <option value="semi-hard">Semi-Hard Negatives</option>
                    <option value="random">Random Negatives</option>
                  </select>
                </div>
              </div>

              <div class="p-4 bg-amber-500/5 border border-amber-500/20 rounded-lg">
                <div class="flex gap-3">
                  <svg xmlns="http://www.w3.org/2000/svg" class="h-5 w-5 text-amber-500 flex-shrink-0" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                    <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M13 16h-1v-4h-1m1-4h.01M21 12a9 9 0 11-18 0 9 9 0 0118 0z" />
                  </svg>
                  <div>
                    <div class="text-sm text-amber-400 font-medium">About Hard Negatives</div>
                    <p class="text-xs text-zinc-500 mt-1">
                      Hard negatives are semantically similar but not matching documents. 
                      They improve model discrimination and are recommended for fine-tuning.
                    </p>
                  </div>
                </div>
              </div>

              <button 
                (click)="exportTriplets()"
                [disabled]="isExporting()"
                class="px-5 py-2.5 bg-violet-600 hover:bg-violet-500 disabled:bg-zinc-800 disabled:text-zinc-500 text-white text-sm font-medium rounded-lg transition-colors flex items-center gap-2"
              >
                <svg xmlns="http://www.w3.org/2000/svg" class="h-4 w-4" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                  <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M4 16v1a3 3 0 003 3h10a3 3 0 003-3v-1m-4-4l-4 4m0 0l-4-4m4 4V4" />
                </svg>
                {{ isExporting() ? 'Generating...' : 'Generate Triplets' }}
              </button>
            </div>
          </section>
        }
      </div>
    </div>
  `
})
export class ExportManagerComponent {
    @Input() collectionName = '';

    private db = inject(VectorDbService);
    private api = inject(HektorApiService);
    private useBackend = environment.useBackend;

    activeTab = signal<'data' | 'pairs' | 'triplets'>('data');
    isExporting = signal(false);
    exportProgress = signal<number | null>(null);

    dataExport = {
        format: 'jsonl' as ExportFormat,
        includeVectors: false,
        dateFrom: '',
        dateTo: ''
    };

    pairsExport = {
        minScore: 0.8
    };

    tripletsExport = {
        negativeSamples: 10,
        strategy: 'hard_negative' as TripletStrategy
    };

    async exportData() {
        this.isExporting.set(true);
        this.exportProgress.set(0);

        try {
            const progressInterval = setInterval(() => {
                const current = this.exportProgress() ?? 0;
                if (current < 90) {
                    this.exportProgress.set(current + Math.random() * 20);
                }
            }, 300);

            if (this.useBackend) {
                const result = await this.api.exportData({
                    outputFile: `${this.collectionName}_export.${this.dataExport.format}`,
                    format: this.dataExport.format,
                    includeVectors: this.dataExport.includeVectors,
                    dateFrom: this.dataExport.dateFrom || undefined,
                    dateTo: this.dataExport.dateTo || undefined
                });
                
                clearInterval(progressInterval);
                this.exportProgress.set(100);
                
                console.log('Export complete:', result);
                
                setTimeout(() => this.exportProgress.set(null), 1000);
            } else {
                const blob = await this.db.exportData(this.collectionName, {
                    format: this.dataExport.format,
                    includeVectors: this.dataExport.includeVectors,
                    dateFrom: this.dataExport.dateFrom || undefined,
                    dateTo: this.dataExport.dateTo || undefined
                });

                clearInterval(progressInterval);
                this.exportProgress.set(100);

                this.downloadBlob(blob, `${this.collectionName}_export.${this.dataExport.format}`);

                setTimeout(() => this.exportProgress.set(null), 1000);
            }
        } catch (error) {
            console.error('Export failed:', error);
            this.exportProgress.set(null);
        } finally {
            this.isExporting.set(false);
        }
    }

    async exportPairs() {
        this.isExporting.set(true);
        try {
            if (this.useBackend) {
                const result = await this.api.exportPairs({
                    outputFile: `${this.collectionName}_pairs.jsonl`,
                    positiveThreshold: this.pairsExport.minScore
                });
                console.log('Pairs export complete:', result);
            } else {
                const blob = await this.db.exportPairs(this.collectionName, this.pairsExport.minScore);
                this.downloadBlob(blob, `${this.collectionName}_pairs.jsonl`);
            }
        } catch (error) {
            console.error('Pairs export failed:', error);
        } finally {
            this.isExporting.set(false);
        }
    }

    async exportTriplets() {
        this.isExporting.set(true);
        try {
            if (this.useBackend) {
                const result = await this.api.exportTriplets({
                    outputFile: `${this.collectionName}_triplets.jsonl`,
                    strategy: this.tripletsExport.strategy,
                    negativeSamples: this.tripletsExport.negativeSamples
                });
                console.log('Triplets export complete:', result);
            } else {
                const blob = await this.db.exportTriplets(this.collectionName, {
                    negativeSamples: this.tripletsExport.negativeSamples,
                    strategy: this.tripletsExport.strategy
                });
                this.downloadBlob(blob, `${this.collectionName}_triplets.jsonl`);
            }
        } catch (error) {
            console.error('Triplets export failed:', error);
        } finally {
            this.isExporting.set(false);
        }
    }

    private downloadBlob(blob: Blob, filename: string) {
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = filename;
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(url);
    }
}
