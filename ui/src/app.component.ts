import { Component, inject, signal, computed, ViewChild } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { VectorDbService } from './services/vector-db.service';
import { AuthService } from './services/auth.service';
import { SchemaField, IngestionConfig } from './models/core';
import { ChatWidgetComponent } from './components/chat-sidebar.component';
import { ProjectionViewComponent } from './components/projection-view.component';
import { SchemaBuilderComponent } from './components/schema-builder.component';
import { HealthMonitorComponent } from './components/health-monitor.component';
import { IngestionWizardComponent } from './components/ingestion-wizard.component';
import { PlaygroundComponent } from './components/playground.component';
import { LoginComponent } from './components/login.component';
// New components
import { HybridSearchComponent } from './components/hybrid-search.component';
import { IndexManagerComponent } from './components/index-manager.component';
import { ExportManagerComponent } from './components/export-manager.component';
import { DatabaseOpsComponent } from './components/database-ops.component';
import { ClusterViewComponent } from './components/cluster-view.component';
import { SettingsComponent } from './components/settings.component';

@Component({
  selector: 'app-root',
  standalone: true,
  imports: [
    CommonModule,
    FormsModule,
    ChatWidgetComponent,
    ProjectionViewComponent,
    SchemaBuilderComponent,
    HealthMonitorComponent,
    IngestionWizardComponent,
    PlaygroundComponent,
    LoginComponent,
    // New components
    HybridSearchComponent,
    IndexManagerComponent,
    ExportManagerComponent,
    DatabaseOpsComponent,
    ClusterViewComponent,
    SettingsComponent
  ],
  templateUrl: './app.component.html'
})
export class AppComponent {
  db = inject(VectorDbService);
  auth = inject(AuthService);

  @ViewChild(ChatWidgetComponent) chatWidget!: ChatWidgetComponent;

  selectedCollection = signal<string | null>(null);
  activeTab = signal<'explore' | 'data' | 'playground' | 'config' | 'hybrid' | 'index' | 'export'>('explore');
  activeMainView = signal<'dashboard' | 'database' | 'cluster' | 'settings'>('dashboard');
  isMobileSidebarOpen = signal(false);
  searchQuery = signal('');

  // Modal State
  isIngestionOpen = signal(false);

  draggedIndex = signal<number | null>(null);

  currentCollection = computed(() => {
    const name = this.selectedCollection();
    return this.db.collections().find(c => c.name === name) || null;
  });

  filteredDocuments = computed(() => {
    const col = this.currentCollection();
    if (!col) return [];

    const query = this.searchQuery().toLowerCase().trim();
    if (!query) return col.documents;

    return col.documents.filter(d =>
      d.id.toLowerCase().includes(query) ||
      d.content.toLowerCase().includes(query) ||
      JSON.stringify(d.metadata).toLowerCase().includes(query)
    );
  });

  selectCollection(name: string) {
    this.selectedCollection.set(name);
    this.searchQuery.set('');
    if (this.activeTab() === 'config') this.activeTab.set('explore');
    this.isMobileSidebarOpen.set(false);
  }

  openDashboard() {
    this.selectedCollection.set(null);
    this.isMobileSidebarOpen.set(false);
  }

  toggleMobileSidebar() {
    this.isMobileSidebarOpen.update(v => !v);
  }

  toggleChat() {
    this.chatWidget.toggle();
    this.isMobileSidebarOpen.set(false);
  }

  openIngestion() {
    this.isIngestionOpen.set(true);
  }

  async handleIngestion(config: IngestionConfig) {
    const result = await this.db.ingestData(config);
    console.log(result); // In a real app, toast notification
    this.isIngestionOpen.set(false);
  }

  onSchemaSave(schema: SchemaField[]) {
    const col = this.currentCollection();
    if (col) {
      this.db.updateSchema(col.name, schema);
    }
  }

  onDragStart(event: DragEvent, index: number) {
    this.draggedIndex.set(index);
    if (event.dataTransfer) {
      event.dataTransfer.effectAllowed = 'move';
      event.dataTransfer.setData('text/plain', index.toString());
    }
  }

  onDragOver(event: DragEvent) {
    event.preventDefault();
    if (event.dataTransfer) {
      event.dataTransfer.dropEffect = 'move';
    }
  }

  onDrop(event: DragEvent, dropIndex: number) {
    event.preventDefault();
    const dragIdx = this.draggedIndex();
    if (dragIdx !== null && dragIdx !== dropIndex) {
      this.db.reorderCollections(dragIdx, dropIndex);
    }
    this.draggedIndex.set(null);
  }

  logout() {
    this.auth.logout();
  }
}