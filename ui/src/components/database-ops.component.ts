import { Component, inject, signal, OnInit } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { VectorDbService } from '../services/vector-db.service';
import { HektorApiService } from '../services/hektor-api.service';
import { DatabaseInfo, HealthStatus } from '../models/core';
import { environment } from '../environments/environment';

@Component({
    selector: 'app-database-ops',
    standalone: true,
    imports: [CommonModule, FormsModule],
    template: `
    <div class="h-full overflow-y-auto custom-scrollbar bg-[#09090b]">
      <div class="max-w-7xl mx-auto p-4 sm:p-6 lg:p-8 space-y-6 sm:space-y-8">
        <!-- Header -->
        <div>
          <h2 class="text-xl sm:text-2xl font-bold text-white mb-1 sm:mb-2">Database Operations</h2>
          <p class="text-sm text-zinc-500">Manage database health, backups, and maintenance</p>
        </div>

        <!-- Database Info -->
        <section class="rounded-xl border border-white/5 bg-white/[0.02] overflow-hidden">
          <div class="px-6 py-4 border-b border-white/5 flex items-center justify-between">
            <h3 class="font-semibold text-white">Database Information</h3>
            <button 
              (click)="refreshInfo()"
              class="text-xs text-zinc-400 hover:text-white transition-colors flex items-center gap-1"
            >
              <svg xmlns="http://www.w3.org/2000/svg" class="h-4 w-4" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M4 4v5h.582m15.356 2A8.001 8.001 0 004.582 9m0 0H9m11 11v-5h-.581m0 0a8.003 8.003 0 01-15.357-2m15.357 2H15" />
              </svg>
              Refresh
            </button>
          </div>
          
          @if (dbInfo()) {
            <div class="p-4 sm:p-6 grid grid-cols-2 sm:grid-cols-3 lg:grid-cols-4 xl:grid-cols-6 gap-3 sm:gap-4 lg:gap-6">
              <div class="col-span-2 sm:col-span-1">
                <div class="text-[10px] sm:text-xs text-zinc-500 uppercase tracking-wider mb-1">Path</div>
                <div class="text-xs sm:text-sm font-mono text-white truncate" [title]="dbInfo()!.path">{{ dbInfo()!.path }}</div>
              </div>
              <div>
                <div class="text-[10px] sm:text-xs text-zinc-500 uppercase tracking-wider mb-1">Vectors</div>
                <div class="text-base sm:text-lg font-mono text-white">{{ dbInfo()!.totalVectors | number }}</div>
              </div>
              <div>
                <div class="text-[10px] sm:text-xs text-zinc-500 uppercase tracking-wider mb-1">Collections</div>
                <div class="text-base sm:text-lg font-mono text-white">{{ dbInfo()!.totalCollections }}</div>
              </div>
              <div>
                <div class="text-[10px] sm:text-xs text-zinc-500 uppercase tracking-wider mb-1">Index</div>
                <div class="text-base sm:text-lg font-mono text-white uppercase">{{ dbInfo()!.indexType }}</div>
              </div>
              <div>
                <div class="text-[10px] sm:text-xs text-zinc-500 uppercase tracking-wider mb-1">Memory</div>
                <div class="text-base sm:text-lg font-mono text-white">{{ formatBytes(dbInfo()!.memoryUsageBytes) }}</div>
              </div>
              <div>
                <div class="text-[10px] sm:text-xs text-zinc-500 uppercase tracking-wider mb-1">Disk</div>
                <div class="text-base sm:text-lg font-mono text-white">{{ formatBytes(dbInfo()!.diskUsageBytes) }}</div>
              </div>
              <div>
                <div class="text-[10px] sm:text-xs text-zinc-500 uppercase tracking-wider mb-1">Version</div>
                <div class="text-xs sm:text-sm font-mono text-indigo-400">{{ dbInfo()!.version }}</div>
              </div>
              @if (dbInfo()!.createdAt) {
                <div>
                  <div class="text-[10px] sm:text-xs text-zinc-500 uppercase tracking-wider mb-1">Created</div>
                  <div class="text-xs sm:text-sm font-mono text-zinc-400">{{ dbInfo()!.createdAt }}</div>
                </div>
              }
              @if (dbInfo()!.modifiedAt) {
                <div>
                  <div class="text-[10px] sm:text-xs text-zinc-500 uppercase tracking-wider mb-1">Modified</div>
                  <div class="text-xs sm:text-sm font-mono text-zinc-400">{{ dbInfo()!.modifiedAt }}</div>
                </div>
              }
            </div>
          } @else {
            <div class="p-6 text-center text-zinc-500">
              <p>Loading database information...</p>
            </div>
          }
        </section>

        <!-- Health Check -->
        <section class="rounded-xl border border-white/5 bg-white/[0.02] overflow-hidden">
          <div class="px-4 sm:px-6 py-3 sm:py-4 border-b border-white/5 flex items-center justify-between">
            <h3 class="text-sm sm:text-base font-semibold text-white">Health Status</h3>
            <button 
              (click)="runHealthCheck()"
              [disabled]="isCheckingHealth()"
              class="px-2 sm:px-3 py-1 sm:py-1.5 bg-emerald-600 hover:bg-emerald-500 disabled:bg-zinc-800 text-white text-[10px] sm:text-xs font-medium rounded transition-colors"
            >
              {{ isCheckingHealth() ? 'Checking...' : 'Run Check' }}
            </button>
          </div>
          
          @if (healthStatus()) {
            <div class="p-4 sm:p-6 space-y-3 sm:space-y-4">
              <!-- Overall Status -->
              <div class="flex items-center gap-2 sm:gap-3">
                <div 
                  class="w-2 h-2 sm:w-3 sm:h-3 rounded-full flex-shrink-0"
                  [class.bg-emerald-500]="healthStatus()!.status === 'healthy'"
                  [class.bg-amber-500]="healthStatus()!.status === 'degraded'"
                  [class.bg-rose-500]="healthStatus()!.status === 'unhealthy'"
                ></div>
                <span 
                  class="text-base sm:text-lg font-semibold capitalize"
                  [class.text-emerald-400]="healthStatus()!.status === 'healthy'"
                  [class.text-amber-400]="healthStatus()!.status === 'degraded'"
                  [class.text-rose-400]="healthStatus()!.status === 'unhealthy'"
                >
                  {{ healthStatus()!.status }}
                </span>
                <span class="text-xs sm:text-sm text-zinc-500 ml-auto">
                  {{ formatUptime(healthStatus()!.uptimeSeconds) }}
                </span>
              </div>

              <!-- Individual Checks -->
              <div class="space-y-1.5 sm:space-y-2">
                @for (check of healthStatus()!.checks; track check.name) {
                  <div class="flex items-center justify-between p-2 sm:p-3 bg-white/[0.02] rounded-lg border border-white/5">
                    <div class="flex items-center gap-2 sm:gap-3 min-w-0 flex-1">
                      @if (check.status === 'pass') {
                        <svg xmlns="http://www.w3.org/2000/svg" class="h-4 w-4 sm:h-5 sm:w-5 text-emerald-500 flex-shrink-0" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                          <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M5 13l4 4L19 7" />
                        </svg>
                      } @else if (check.status === 'warn') {
                        <svg xmlns="http://www.w3.org/2000/svg" class="h-4 w-4 sm:h-5 sm:w-5 text-amber-500 flex-shrink-0" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                          <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 9v2m0 4h.01m-6.938 4h13.856c1.54 0 2.502-1.667 1.732-3L13.732 4c-.77-1.333-2.694-1.333-3.464 0L3.34 16c-.77 1.333.192 3 1.732 3z" />
                        </svg>
                      } @else {
                        <svg xmlns="http://www.w3.org/2000/svg" class="h-4 w-4 sm:h-5 sm:w-5 text-rose-500 flex-shrink-0" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                          <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M6 18L18 6M6 6l12 12" />
                        </svg>
                      }
                      <span class="text-xs sm:text-sm text-zinc-300 truncate">{{ check.name }}</span>
                    </div>
                    @if (check.message) {
                      <span class="text-[10px] sm:text-xs text-zinc-500 ml-2 hidden sm:inline">{{ check.message }}</span>
                    }
                  </div>
                }
              </div>
            </div>
          } @else {
            <div class="p-6 text-center text-zinc-500">
              <p>Click "Run Health Check" to check database status</p>
            </div>
          }
        </section>

        <!-- Backup & Restore -->
        <section class="rounded-xl border border-white/5 bg-white/[0.02] overflow-hidden">
          <div class="px-4 sm:px-6 py-3 sm:py-4 border-b border-white/5">
            <h3 class="text-sm sm:text-base font-semibold text-white">Backup & Restore</h3>
          </div>
          <div class="p-4 sm:p-6 space-y-4 sm:space-y-6">
            <div class="grid grid-cols-1 lg:grid-cols-2 gap-4 sm:gap-6">
              <!-- Backup Section -->
              <div class="space-y-3 sm:space-y-4">
                <h4 class="text-xs sm:text-sm font-semibold text-zinc-300">Create Backup</h4>
                <div>
                  <label class="block text-[10px] sm:text-xs font-bold text-zinc-500 uppercase tracking-wider mb-1.5 sm:mb-2">Destination</label>
                  <input 
                    type="text" 
                    [(ngModel)]="backupPath"
                    placeholder="./backups/backup_2026_01_09"
                    class="w-full bg-zinc-900/50 border border-white/10 rounded-lg px-3 sm:px-4 py-2 sm:py-2.5 text-white text-xs sm:text-sm focus:outline-none focus:border-indigo-500/50 placeholder-zinc-600"
                  />
                </div>
                <div>
                  <label class="flex items-center gap-2 sm:gap-3 cursor-pointer">
                    <input 
                      type="checkbox" 
                      [(ngModel)]="compressBackup"
                      class="w-4 h-4 sm:w-5 sm:h-5 rounded bg-zinc-800 border-zinc-700 text-indigo-500 focus:ring-indigo-500 focus:ring-offset-0"
                    />
                    <span class="text-xs sm:text-sm text-zinc-300">Compress (tar.gz)</span>
                  </label>
                </div>
                <button 
                  (click)="createBackup()"
                  [disabled]="isBackingUp() || !backupPath.trim()"
                  class="w-full px-3 sm:px-4 py-2 sm:py-2.5 bg-indigo-600 hover:bg-indigo-500 disabled:bg-zinc-800 disabled:text-zinc-500 text-white text-xs sm:text-sm font-medium rounded-lg transition-colors"
                >
                  {{ isBackingUp() ? 'Creating...' : 'Create Backup' }}
                </button>
              </div>

              <!-- Restore Section -->
              <div class="space-y-3 sm:space-y-4">
                <h4 class="text-xs sm:text-sm font-semibold text-zinc-300">Restore from Backup</h4>
                <div>
                  <label class="block text-[10px] sm:text-xs font-bold text-zinc-500 uppercase tracking-wider mb-1.5 sm:mb-2">Source</label>
                  <input 
                    type="text" 
                    [(ngModel)]="restorePath"
                    placeholder="./backups/backup_2026_01_09.tar.gz"
                    class="w-full bg-zinc-900/50 border border-white/10 rounded-lg px-3 sm:px-4 py-2 sm:py-2.5 text-white text-xs sm:text-sm focus:outline-none focus:border-indigo-500/50 placeholder-zinc-600"
                  />
                </div>
                <div class="p-2 sm:p-3 bg-rose-500/5 border border-rose-500/20 rounded-lg">
                  <p class="text-[10px] sm:text-xs text-rose-400">
                    ⚠️ Restore overwrites current database
                  </p>
                </div>
                <button 
                  (click)="restoreBackup()"
                  [disabled]="isRestoring() || !restorePath.trim()"
                  class="w-full px-3 sm:px-4 py-2 sm:py-2.5 bg-rose-600 hover:bg-rose-500 disabled:bg-zinc-800 disabled:text-zinc-500 text-white text-xs sm:text-sm font-medium rounded-lg transition-colors"
                >
                  {{ isRestoring() ? 'Restoring...' : 'Restore Backup' }}
                </button>
              </div>
            </div>
          </div>
        </section>

        <!-- Maintenance -->
        <section class="rounded-xl border border-white/5 bg-white/[0.02] overflow-hidden">
          <div class="px-4 sm:px-6 py-3 sm:py-4 border-b border-white/5">
            <h3 class="text-sm sm:text-base font-semibold text-white">Maintenance</h3>
          </div>
          <div class="p-4 sm:p-6">
            <div class="flex flex-col sm:flex-row flex-wrap gap-2 sm:gap-4">
              <button 
                (click)="optimizeDb()"
                [disabled]="isOptimizing()"
                class="px-4 sm:px-5 py-2 sm:py-2.5 bg-white/5 hover:bg-white/10 text-zinc-300 text-xs sm:text-sm font-medium rounded-lg transition-colors border border-white/5 flex items-center justify-center gap-2"
              >
                <svg xmlns="http://www.w3.org/2000/svg" class="h-3.5 w-3.5 sm:h-4 sm:w-4" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                  <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M13 10V3L4 14h7v7l9-11h-7z" />
                </svg>
                {{ isOptimizing() ? 'Optimizing...' : 'Optimize Database' }}
              </button>
              <button 
                (click)="syncDb()"
                [disabled]="isSyncing()"
                class="px-4 sm:px-5 py-2 sm:py-2.5 bg-white/5 hover:bg-white/10 text-zinc-300 text-xs sm:text-sm font-medium rounded-lg transition-colors border border-white/5 flex items-center justify-center gap-2"
              >
                <svg xmlns="http://www.w3.org/2000/svg" class="h-3.5 w-3.5 sm:h-4 sm:w-4" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                  <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M4 4v5h.582m15.356 2A8.001 8.001 0 004.582 9m0 0H9m11 11v-5h-.581m0 0a8.003 8.003 0 01-15.357-2m15.357 2H15" />
                </svg>
                {{ isSyncing() ? 'Syncing...' : 'Sync to Disk' }}
              </button>
            </div>
          </div>
        </section>
      </div>
    </div>
  `
})
export class DatabaseOpsComponent implements OnInit {
    private db = inject(VectorDbService);
    private api = inject(HektorApiService);
    private useBackend = environment.useBackend;

    dbInfo = signal<DatabaseInfo | null>(null);
    healthStatus = signal<HealthStatus | null>(null);

    isCheckingHealth = signal(false);
    isBackingUp = signal(false);
    isRestoring = signal(false);
    isOptimizing = signal(false);
    isSyncing = signal(false);

    backupPath = '';
    restorePath = '';
    compressBackup = true;

    ngOnInit() {
        this.refreshInfo();
        if (this.useBackend) {
            this.runHealthCheck();
        }
    }

    async refreshInfo() {
        try {
            if (this.useBackend) {
                const info = await this.api.getDatabaseInfo();
                this.dbInfo.set(info);
            } else {
                // Mock data for development
                this.dbInfo.set({
                    path: './vectors',
                    totalVectors: this.db.stats().totalDocs,
                    totalCollections: this.db.collections().length,
                    indexType: 'HNSW',
                    memoryUsageBytes: parseInt(this.db.stats().memoryUsage) * 1024 * 1024,
                    diskUsageBytes: parseInt(this.db.stats().memoryUsage) * 1024 * 1024 * 2,
                    version: '3.0.0',
                    createdAt: '2026-01-01',
                    modifiedAt: new Date().toISOString().split('T')[0]
                });
            }
        } catch (error) {
            console.error('Failed to fetch database info:', error);
        }
    }

    async runHealthCheck() {
        this.isCheckingHealth.set(true);
        try {
            if (this.useBackend) {
                const status = await this.api.getHealthStatus();
                this.healthStatus.set(status);
            } else {
                // Mock health check for development
                this.healthStatus.set({
                    status: 'healthy',
                    uptimeSeconds: 86400,
                    checks: [
                        { name: 'Database Connection', status: 'pass', message: 'Connected' },
                        { name: 'Memory Usage', status: 'pass', message: 'Normal' },
                        { name: 'Disk Space', status: 'pass', message: '85% available' },
                        { name: 'Index Integrity', status: 'pass', message: 'Valid' },
                    ]
                });
            }
        } catch (error) {
            console.error('Health check failed:', error);
            this.healthStatus.set({
                status: 'unhealthy',
                uptimeSeconds: 0,
                checks: [
                    { name: 'Backend Connection', status: 'fail', message: 'Cannot connect to API' }
                ]
            });
        } finally {
            this.isCheckingHealth.set(false);
        }
    }

    async createBackup() {
        this.isBackingUp.set(true);
        try {
            if (this.useBackend) {
                const result = await this.api.backupDatabase(this.backupPath);
                console.log('Backup created:', result);
            } else {
                // Simulate backup
                await new Promise(resolve => setTimeout(resolve, 1000));
                console.log('Mock backup created to:', this.backupPath);
            }
            this.backupPath = '';
        } catch (error) {
            console.error('Backup failed:', error);
        } finally {
            this.isBackingUp.set(false);
        }
    }

    async restoreBackup() {
        if (!confirm('Are you sure you want to restore from backup? This will overwrite the current database.')) {
            return;
        }

        this.isRestoring.set(true);
        try {
            if (this.useBackend) {
                const result = await this.api.restoreDatabase(this.restorePath);
                console.log('Database restored:', result);
            } else {
                // Simulate restore
                await new Promise(resolve => setTimeout(resolve, 2000));
                console.log('Mock restore from:', this.restorePath);
            }
            await this.refreshInfo();
            this.restorePath = '';
        } catch (error) {
            console.error('Restore failed:', error);
        } finally {
            this.isRestoring.set(false);
        }
    }

    async optimizeDb() {
        this.isOptimizing.set(true);
        try {
            if (this.useBackend) {
                const result = await this.api.optimizeDatabase();
                console.log('Database optimized:', result);
            } else {
                // Simulate optimize
                await new Promise(resolve => setTimeout(resolve, 1500));
                console.log('Mock database optimization complete');
            }
            await this.refreshInfo();
        } catch (error) {
            console.error('Optimize failed:', error);
        } finally {
            this.isOptimizing.set(false);
        }
    }

    async syncDb() {
        this.isSyncing.set(true);
        try {
            if (this.useBackend) {
                // Sync is typically part of optimize or happens automatically
                await this.api.optimizeDatabase();
            } else {
                // Simulate sync
                await new Promise(resolve => setTimeout(resolve, 500));
                console.log('Mock sync to disk complete');
            }
        } catch (error) {
            console.error('Sync failed:', error);
        } finally {
            this.isSyncing.set(false);
        }
    }

    formatBytes(bytes: number): string {
        if (bytes === 0) return '0 B';
        const k = 1024;
        const sizes = ['B', 'KB', 'MB', 'GB'];
        const i = Math.floor(Math.log(bytes) / Math.log(k));
        return parseFloat((bytes / Math.pow(k, i)).toFixed(1)) + ' ' + sizes[i];
    }

    formatUptime(seconds: number): string {
        const days = Math.floor(seconds / 86400);
        const hours = Math.floor((seconds % 86400) / 3600);
        const mins = Math.floor((seconds % 3600) / 60);

        if (days > 0) return `${days}d ${hours}h`;
        if (hours > 0) return `${hours}h ${mins}m`;
        return `${mins}m`;
    }
}
