import { Component, inject, signal, computed, ViewChild } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { VectorDbService, SchemaField } from './services/vector-db.service';
import { ChatWidgetComponent } from './components/chat-sidebar.component';
import { ProjectionViewComponent } from './components/projection-view.component';
import { SchemaBuilderComponent } from './components/schema-builder.component';
import { HealthMonitorComponent } from './components/health-monitor.component';

@Component({
  selector: 'app-root',
  standalone: true,
  imports: [CommonModule, FormsModule, ChatWidgetComponent, ProjectionViewComponent, SchemaBuilderComponent, HealthMonitorComponent],
  templateUrl: './app.component.html'
})
export class AppComponent {
  db = inject(VectorDbService);
  
  @ViewChild(ChatWidgetComponent) chatWidget!: ChatWidgetComponent;

  selectedCollection = signal<string | null>(null);
  activeTab = signal<'explore' | 'data' | 'config'>('explore');
  isMobileSidebarOpen = signal(false);
  searchQuery = signal('');
  
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
    this.searchQuery.set(''); // Reset search on collection change
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

  onSchemaSave(schema: SchemaField[]) {
    const col = this.currentCollection();
    if (col) {
      this.db.updateSchema(col.name, schema);
    }
  }

  // Drag and Drop Handlers
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
}