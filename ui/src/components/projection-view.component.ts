import { Component, ElementRef, ViewChild, effect, input, OnDestroy, signal, AfterViewInit } from '@angular/core';
import { CommonModule } from '@angular/common';
import { VectorDoc } from '../models/core';

declare const d3: any;
declare const THREE: any;

@Component({
  selector: 'app-projection-view',
  standalone: true,
  imports: [CommonModule],
  template: `
    <div class="w-full h-full relative overflow-hidden bg-[#050505] select-none group">
      
      <!-- Visualization Container -->
      <div #chartContainer class="w-full h-full absolute inset-0 outline-none" style="touch-action: none;"></div>
      
      <!-- Tooltip for Node Details -->
      <div #tooltip class="absolute hidden z-30 bg-zinc-900/95 border border-zinc-700 rounded-lg shadow-2xl backdrop-blur-md p-4 max-w-sm pointer-events-none">
        <div class="space-y-2">
          <div class="flex items-start justify-between gap-3 pb-2 border-b border-zinc-700">
            <div class="flex-1 min-w-0">
              <h4 class="text-xs font-bold text-zinc-400 uppercase tracking-wider mb-1">Document ID</h4>
              <p class="text-sm text-white font-mono truncate" [title]="hoveredNode()?.id || ''">{{ hoveredNode()?.id || '' }}</p>
            </div>
            <div class="flex-shrink-0">
              <span class="inline-flex items-center px-2 py-1 rounded text-[10px] font-bold bg-indigo-500/20 text-indigo-300">
                Cluster {{ hoveredNode()?.cluster ?? 'N/A' }}
              </span>
            </div>
          </div>
          
          <div>
            <h4 class="text-xs font-bold text-zinc-400 uppercase tracking-wider mb-1">Content Preview</h4>
            <p class="text-xs text-zinc-300 line-clamp-3">{{ hoveredNode()?.content || 'No content' }}</p>
          </div>
          
          <div *ngIf="hoveredNode()?.score !== undefined">
            <h4 class="text-xs font-bold text-zinc-400 uppercase tracking-wider mb-1">Similarity Score</h4>
            <div class="flex items-center gap-2">
              <div class="flex-1 h-2 bg-zinc-800 rounded-full overflow-hidden">
                <div class="h-full bg-gradient-to-r from-indigo-500 to-purple-500 rounded-full transition-all duration-300" 
                     [style.width.%]="(hoveredNode()?.score || 0) * 100"></div>
              </div>
              <span class="text-xs font-mono text-white">{{ ((hoveredNode()?.score || 0) * 100).toFixed(1) }}%</span>
            </div>
          </div>
          
          <div *ngIf="hoveredNode()?.metadata && getMetadataEntries(hoveredNode()?.metadata).length > 0">
            <h4 class="text-xs font-bold text-zinc-400 uppercase tracking-wider mb-1">Metadata</h4>
            <div class="space-y-1 max-h-32 overflow-y-auto custom-scrollbar">
              <div *ngFor="let item of getMetadataEntries(hoveredNode()?.metadata)" 
                   class="flex items-start justify-between gap-2 text-xs">
                <span class="text-zinc-500 font-mono">{{ item.key }}:</span>
                <span class="text-zinc-300 font-mono text-right truncate">{{ formatValue(item.value) }}</span>
              </div>
            </div>
          </div>
          
          <div>
            <h4 class="text-xs font-bold text-zinc-400 uppercase tracking-wider mb-1">Vector Space Position</h4>
            <div class="grid grid-cols-2 gap-2 text-xs font-mono">
              <div class="flex justify-between">
                <span class="text-zinc-500">X:</span>
                <span class="text-zinc-300">{{ hoveredNode()?.projection?.[0]?.toFixed(2) || 'N/A' }}</span>
              </div>
              <div class="flex justify-between">
                <span class="text-zinc-500">Y:</span>
                <span class="text-zinc-300">{{ hoveredNode()?.projection?.[1]?.toFixed(2) || 'N/A' }}</span>
              </div>
              <div *ngIf="mode() === '3d'" class="flex justify-between col-span-2">
                <span class="text-zinc-500">Z:</span>
                <span class="text-zinc-300">{{ hoveredNode()?.projection3d?.[2]?.toFixed(2) || 'N/A' }}</span>
              </div>
            </div>
          </div>
        </div>
      </div>
      
      <!-- Overlay UI -->
      <div class="absolute top-4 right-4 z-20 flex flex-col items-end gap-2 pointer-events-none">
        <!-- Mode Switcher -->
        <div class="bg-zinc-900/80 border border-zinc-800 rounded-lg p-1 backdrop-blur-md shadow-2xl pointer-events-auto flex flex-col gap-1">
          <div class="flex">
            <button (click)="setMode('2d')" 
                class="px-4 py-1.5 text-xs font-bold rounded-md transition-all duration-300"
                [class.bg-zinc-700]="mode() === '2d'"
                [class.text-white]="mode() === '2d'"
                [class.text-zinc-500]="mode() !== '2d'">2D</button>
            <button (click)="setMode('3d')" 
                class="px-4 py-1.5 text-xs font-bold rounded-md transition-all duration-300"
                [class.bg-zinc-700]="mode() === '3d'"
                [class.text-white]="mode() === '3d'"
                [class.text-zinc-500]="mode() !== '3d'">3D</button>
          </div>
          
          <div class="text-[9px] text-zinc-600 text-center font-mono border-t border-white/5 pt-1 mt-1">
            {{ mode() === '3d' ? 'Left Click: Rotate • Right: Pan' : 'Scroll: Zoom • Drag: Pan' }}
          </div>
        </div>
        
        <!-- Stats Panel -->
        <div class="bg-zinc-900/80 border border-zinc-800 rounded-lg p-3 backdrop-blur-md shadow-2xl pointer-events-auto">
          <div class="space-y-2 text-xs">
            <div class="flex items-center justify-between gap-4">
              <span class="text-zinc-500">Vectors:</span>
              <span class="text-white font-mono">{{ documents().length }}</span>
            </div>
            <div class="flex items-center justify-between gap-4">
              <span class="text-zinc-500">Clusters:</span>
              <span class="text-white font-mono">{{ getUniqueClusters() }}</span>
            </div>
            <div class="flex items-center justify-between gap-4">
              <span class="text-zinc-500">Dimension:</span>
              <span class="text-white font-mono">{{ documents()[0]?.vector?.length || 'N/A' }}</span>
            </div>
          </div>
        </div>
      </div>
      
      <!-- Loading State -->
      <div *ngIf="loading()" class="absolute inset-0 flex items-center justify-center bg-black/50 z-10 backdrop-blur-sm transition-opacity duration-500">
        <div class="flex flex-col items-center gap-4">
           <div class="relative w-16 h-16">
              <div class="absolute inset-0 border-4 border-indigo-500/30 rounded-full"></div>
              <div class="absolute inset-0 border-4 border-indigo-500 border-t-transparent rounded-full animate-spin"></div>
           </div>
           <p class="text-xs font-mono text-indigo-400 animate-pulse">RENDERING VECTOR SPACE...</p>
        </div>
      </div>
    </div>
  `,
  styles: [`
    .custom-scrollbar::-webkit-scrollbar {
      width: 4px;
    }
    .custom-scrollbar::-webkit-scrollbar-track {
      background: rgba(255, 255, 255, 0.05);
      border-radius: 2px;
    }
    .custom-scrollbar::-webkit-scrollbar-thumb {
      background: rgba(255, 255, 255, 0.2);
      border-radius: 2px;
    }
    .custom-scrollbar::-webkit-scrollbar-thumb:hover {
      background: rgba(255, 255, 255, 0.3);
    }
  `]
})
export class ProjectionViewComponent implements AfterViewInit, OnDestroy {
  documents = input.required<VectorDoc[]>();
  mode = signal<'2d' | '3d'>('2d');
  loading = signal(false);
  hoveredNode = signal<VectorDoc | null>(null);

  @ViewChild('chartContainer') container!: ElementRef;
  @ViewChild('tooltip') tooltipElement!: ElementRef;

  private resizeObserver: ResizeObserver | null = null;
  private renderer: any; // THREE.WebGLRenderer
  private scene: any;
  private camera: any;
  private controls: any;
  private pointsMesh: any;
  private requestID: number | null = null;
  private d3Svg: any;
  private raycaster: any;
  private mouse: any;

  constructor() {
    effect(() => {
      const docs = this.documents();
      const m = this.mode();
      
      // Debounce slightly to allow DOM init
      setTimeout(() => {
        if (this.container?.nativeElement) {
            this.updateVisualization(docs, m);
        }
      }, 50);
    });
  }

  getMetadataEntries(metadata: Record<string, any> | undefined): { key: string; value: any }[] {
    if (!metadata) return [];
    return Object.entries(metadata).map(([key, value]) => ({ key, value }));
  }

  formatValue(value: any): string {
    if (value === null || value === undefined) return 'null';
    if (typeof value === 'object') return JSON.stringify(value);
    if (typeof value === 'number') return value.toFixed(2);
    return String(value);
  }

  getUniqueClusters(): number {
    const clusters = new Set(this.documents().map(d => d.cluster));
    return clusters.size;
  }

  ngAfterViewInit() {
    this.resizeObserver = new ResizeObserver(() => {
       this.handleResize();
    });
    this.resizeObserver.observe(this.container.nativeElement);
  }

  ngOnDestroy() {
    this.cleanup();
    this.resizeObserver?.disconnect();
  }

  setMode(m: '2d' | '3d') {
    this.mode.set(m);
  }

  private handleResize() {
     // Re-trigger update to adjust scales/camera aspect ratio
     this.updateVisualization(this.documents(), this.mode());
  }

  private cleanup() {
    if (this.requestID) cancelAnimationFrame(this.requestID);
    if (this.renderer) {
      this.renderer.dispose();
      this.container.nativeElement.removeChild(this.renderer.domElement);
    }
    if (this.controls) {
        this.controls.dispose();
    }
    if (this.d3Svg) {
        this.d3Svg.remove();
    }
    this.scene = null;
    this.camera = null;
    this.renderer = null;
    this.controls = null;
  }

  private updateVisualization(docs: VectorDoc[], mode: '2d' | '3d') {
    if (!docs.length) return;

    this.cleanup();
    
    if (mode === '2d') {
        this.render2D(docs);
    } else {
        this.render3D(docs);
    }
  }

  private render2D(docs: VectorDoc[]) {
    const el = this.container.nativeElement;
    const width = el.clientWidth;
    const height = el.clientHeight;

    this.d3Svg = d3.select(el)
      .append('svg')
      .attr('width', width)
      .attr('height', height)
      .style('background', '#050505')
      .style('cursor', 'move'); // Indicate pan/zoom capability
    
    // Create a group for content that will be transformed
    const g = this.d3Svg.append("g");
    
    // Initialize zoom behavior
    const zoom = d3.zoom()
        .scaleExtent([0.5, 20]) // Zoom limits
        .on("zoom", (e: any) => {
            g.attr("transform", e.transform);
        });
        
    this.d3Svg.call(zoom);

    // Safety: Handle single point or zero variance domains
    const xExtent = d3.extent(docs, (d: VectorDoc) => d.projection[0]);
    const yExtent = d3.extent(docs, (d: VectorDoc) => d.projection[1]);
    
    let [minX, maxX] = xExtent;
    let [minY, maxY] = yExtent;

    // Add padding if min equals max
    if (minX === maxX) { minX -= 1; maxX += 1; }
    if (minY === maxY) { minY -= 1; maxY += 1; }

    const x = d3.scaleLinear()
      .domain([minX, maxX])
      .range([50, width - 50]);

    const y = d3.scaleLinear()
      .domain([minY, maxY])
      .range([height - 50, 50]);

    // Color scale based on cluster
    const color = d3.scaleOrdinal(d3.schemeCategory10);

    const circles = g.selectAll("circle")
      .data(docs)
      .enter()
      .append("circle")
      .attr("cx", (d: VectorDoc) => x(d.projection[0]))
      .attr("cy", (d: VectorDoc) => y(d.projection[1]))
      .attr("r", 4)
      .attr("fill", (d: VectorDoc) => color(d.cluster || 0))
      .attr("opacity", 0.7)
      .attr("stroke", "#000")
      .attr("stroke-width", 1)
      .style("cursor", "pointer")
      .on("mouseover", (event: any, d: VectorDoc) => {
         d3.select(event.currentTarget).transition().duration(200).attr("r", 8).attr("opacity", 1);
         this.showTooltip(event, d);
      })
      .on("mousemove", (event: any, d: VectorDoc) => {
         this.updateTooltipPosition(event);
      })
      .on("mouseout", (event: any, d: VectorDoc) => {
         d3.select(event.currentTarget).transition().duration(200).attr("r", 4).attr("opacity", 0.7);
         this.hideTooltip();
      })
      .on("click", (event: any, d: VectorDoc) => {
         // Optional: Add click handler for more detailed view
         console.log('Node clicked:', d);
      });
      
    // Add connection lines between nearby nodes in same cluster (optional enhancement)
    if (docs.length < 200) { // Only for smaller datasets to avoid performance issues
      const connections = this.computeNearbyConnections(docs, 3); // Max 3 connections per node
      g.selectAll("line")
        .data(connections)
        .enter()
        .append("line")
        .attr("x1", (d: any) => x(d.source.projection[0]))
        .attr("y1", (d: any) => y(d.source.projection[1]))
        .attr("x2", (d: any) => x(d.target.projection[0]))
        .attr("y2", (d: any) => y(d.target.projection[1]))
        .attr("stroke", (d: any) => color(d.source.cluster || 0))
        .attr("stroke-width", 0.5)
        .attr("opacity", 0.2);
    }
  }

  private computeNearbyConnections(docs: VectorDoc[], maxPerNode: number): any[] {
    const connections: any[] = [];
    docs.forEach((doc, i) => {
      let added = 0;
      for (let j = i + 1; j < docs.length && added < maxPerNode; j++) {
        if (docs[j].cluster === doc.cluster) {
          const dist = Math.hypot(
            doc.projection[0] - docs[j].projection[0],
            doc.projection[1] - docs[j].projection[1]
          );
          if (dist < 5) { // Threshold for "nearby"
            connections.push({ source: doc, target: docs[j] });
            added++;
          }
        }
      }
    });
    return connections;
  }

  private showTooltip(event: any, node: VectorDoc) {
    this.hoveredNode.set(node);
    const tooltip = this.tooltipElement.nativeElement;
    tooltip.classList.remove('hidden');
    this.updateTooltipPosition(event);
  }

  private updateTooltipPosition(event: any) {
    const tooltip = this.tooltipElement.nativeElement;
    const containerRect = this.container.nativeElement.getBoundingClientRect();
    
    let x = event.clientX - containerRect.left + 15;
    let y = event.clientY - containerRect.top + 15;
    
    // Prevent tooltip from going off-screen
    const tooltipRect = tooltip.getBoundingClientRect();
    if (x + tooltipRect.width > containerRect.width) {
      x = event.clientX - containerRect.left - tooltipRect.width - 15;
    }
    if (y + tooltipRect.height > containerRect.height) {
      y = event.clientY - containerRect.top - tooltipRect.height - 15;
    }
    
    tooltip.style.left = `${x}px`;
    tooltip.style.top = `${y}px`;
  }

  private hideTooltip() {
    this.hoveredNode.set(null);
    const tooltip = this.tooltipElement.nativeElement;
    tooltip.classList.add('hidden');
  }

  private render3D(docs: VectorDoc[]) {
    const el = this.container.nativeElement;
    const width = el.clientWidth;
    const height = el.clientHeight;

    this.scene = new THREE.Scene();
    this.scene.background = new THREE.Color(0x050505);
    this.scene.fog = new THREE.FogExp2(0x050505, 0.002); // Distance fog

    this.camera = new THREE.PerspectiveCamera(60, width / height, 0.1, 1000);
    this.camera.position.z = 100;

    this.renderer = new THREE.WebGLRenderer({ antialias: true });
    this.renderer.setSize(width, height);
    this.renderer.setPixelRatio(window.devicePixelRatio);
    el.appendChild(this.renderer.domElement);

    // Initialize OrbitControls
    if (THREE.OrbitControls) {
        this.controls = new THREE.OrbitControls(this.camera, this.renderer.domElement);
        this.controls.enableDamping = true; 
        this.controls.dampingFactor = 0.05;
        this.controls.screenSpacePanning = true;
        this.controls.minDistance = 5;
        this.controls.maxDistance = 500;
        
        // Auto-rotate gently until user interacts
        this.controls.autoRotate = true;
        this.controls.autoRotateSpeed = 0.5;

        // Stop auto-rotation on user interaction
        this.controls.addEventListener('start', () => {
            this.controls.autoRotate = false;
        });
    }

    // Raycaster for mouse picking
    this.raycaster = new THREE.Raycaster();
    this.raycaster.params.Points.threshold = 2;
    this.mouse = new THREE.Vector2();

    // Geometry - Create individual spheres for better interaction
    const colorScale = d3.scaleOrdinal(d3.schemeCategory10);
    const spheres: any[] = [];

    docs.forEach((doc, index) => {
      // Safety check for missing 3D projection
      const p = doc.projection3d || [doc.projection[0], doc.projection[1], 0];
      
      const geometry = new THREE.SphereGeometry(1.5, 16, 16);
      const color = new THREE.Color(colorScale(doc.cluster || 0));
      const material = new THREE.MeshBasicMaterial({ 
        color: color,
        transparent: true,
        opacity: 0.8
      });
      
      const sphere = new THREE.Mesh(geometry, material);
      sphere.position.set(p[0], p[1], p[2]);
      sphere.userData = { doc, index, originalColor: color.clone() };
      
      this.scene.add(sphere);
      spheres.push(sphere);
    });

    // Add connections between nearby nodes
    if (docs.length < 200) {
      const connections = this.computeNearbyConnections(docs, 3);
      connections.forEach((conn: any) => {
        const p1 = conn.source.projection3d || [conn.source.projection[0], conn.source.projection[1], 0];
        const p2 = conn.target.projection3d || [conn.target.projection[0], conn.target.projection[1], 0];
        
        const geometry = new THREE.BufferGeometry().setFromPoints([
          new THREE.Vector3(p1[0], p1[1], p1[2]),
          new THREE.Vector3(p2[0], p2[1], p2[2])
        ]);
        const material = new THREE.LineBasicMaterial({ 
          color: new THREE.Color(colorScale(conn.source.cluster || 0)),
          transparent: true,
          opacity: 0.2
        });
        const line = new THREE.Line(geometry, material);
        this.scene.add(line);
      });
    }

    // Mouse interaction handlers
    let hoveredObject: any = null;
    
    const onMouseMove = (event: MouseEvent) => {
      const rect = this.renderer.domElement.getBoundingClientRect();
      this.mouse.x = ((event.clientX - rect.left) / rect.width) * 2 - 1;
      this.mouse.y = -((event.clientY - rect.top) / rect.height) * 2 + 1;
      
      this.raycaster.setFromCamera(this.mouse, this.camera);
      const intersects = this.raycaster.intersectObjects(spheres);
      
      if (intersects.length > 0) {
        const object = intersects[0].object;
        
        if (hoveredObject !== object) {
          // Reset previous hovered object
          if (hoveredObject) {
            hoveredObject.material.opacity = 0.8;
            hoveredObject.scale.set(1, 1, 1);
          }
          
          // Highlight new object
          hoveredObject = object;
          hoveredObject.material.opacity = 1.0;
          hoveredObject.scale.set(1.5, 1.5, 1.5);
          
          // Show tooltip
          this.showTooltip(event, object.userData.doc);
        }
        
        this.updateTooltipPosition(event);
      } else {
        if (hoveredObject) {
          hoveredObject.material.opacity = 0.8;
          hoveredObject.scale.set(1, 1, 1);
          hoveredObject = null;
          this.hideTooltip();
        }
      }
    };
    
    const onClick = (event: MouseEvent) => {
      if (hoveredObject) {
        console.log('Node clicked:', hoveredObject.userData.doc);
      }
    };
    
    this.renderer.domElement.addEventListener('mousemove', onMouseMove);
    this.renderer.domElement.addEventListener('click', onClick);

    // Animation Loop
    const animate = () => {
      this.requestID = requestAnimationFrame(animate);
      
      if (this.controls) {
          this.controls.update();
      } else {
          // Fallback if no controls loaded
           if (this.pointsMesh) {
              this.pointsMesh.rotation.y += 0.001;
           }
      }

      this.renderer.render(this.scene, this.camera);
    };

    animate();
  }
}