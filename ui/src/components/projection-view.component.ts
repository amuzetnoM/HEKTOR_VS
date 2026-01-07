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
  `
})
export class ProjectionViewComponent implements AfterViewInit, OnDestroy {
  documents = input.required<VectorDoc[]>();
  mode = signal<'2d' | '3d'>('2d');
  loading = signal(false);

  @ViewChild('chartContainer') container!: ElementRef;

  private resizeObserver: ResizeObserver | null = null;
  private renderer: any; // THREE.WebGLRenderer
  private scene: any;
  private camera: any;
  private controls: any;
  private pointsMesh: any;
  private requestID: number | null = null;
  private d3Svg: any;

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

    g.selectAll("circle")
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
      .on("mouseover", function(event: any, d: VectorDoc) {
         d3.select(this).transition().duration(200).attr("r", 8).attr("opacity", 1);
      })
      .on("mouseout", function(event: any, d: VectorDoc) {
         d3.select(this).transition().duration(200).attr("r", 4).attr("opacity", 0.7);
      });
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

    // Geometry
    const geometry = new THREE.BufferGeometry();
    const positions: number[] = [];
    const colors: number[] = [];
    
    const colorScale = d3.scaleOrdinal(d3.schemeCategory10);
    const colorObj = new THREE.Color();

    docs.forEach(d => {
        // Safety check for missing 3D projection
        const p = d.projection3d || [d.projection[0], d.projection[1], 0];
        
        positions.push(p[0], p[1], p[2]);
        
        colorObj.set(colorScale(d.cluster || 0));
        colors.push(colorObj.r, colorObj.g, colorObj.b);
    });

    geometry.setAttribute('position', new THREE.Float32BufferAttribute(positions, 3));
    geometry.setAttribute('color', new THREE.Float32BufferAttribute(colors, 3));

    const material = new THREE.PointsMaterial({ 
        size: 3, 
        vertexColors: true,
        sizeAttenuation: true,
        transparent: true,
        opacity: 0.8
    });

    this.pointsMesh = new THREE.Points(geometry, material);
    this.scene.add(this.pointsMesh);

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