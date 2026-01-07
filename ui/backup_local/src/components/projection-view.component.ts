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
        <div class="bg-zinc-900/80 border border-zinc-800 rounded-lg p-1 backdrop-blur-md shadow-2xl pointer-events-auto">
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
      </div>

      <!-- Controls Hint -->
      <div class="absolute bottom-6 left-1/2 -translate-x-1/2 flex items-center gap-6 px-4 py-2 rounded-full bg-zinc-900/50 border border-white/5 backdrop-blur-sm text-[10px] font-mono text-zinc-500 pointer-events-none opacity-50 hover:opacity-100 transition-opacity whitespace-nowrap z-10">
        <span class="flex items-center gap-1.5"><span class="w-1.5 h-1.5 rounded-full bg-indigo-500"></span> SELECT (Click)</span>
        @if (mode() === '3d') {
          <span class="flex items-center gap-1.5"><span class="w-1.5 h-1.5 rounded-full bg-emerald-500"></span> ROTATE (Drag)</span>
          <span class="flex items-center gap-1.5"><span class="w-1.5 h-1.5 rounded-full bg-violet-500"></span> PAN (Right-Click Drag)</span>
        } @else {
          <span class="flex items-center gap-1.5"><span class="w-1.5 h-1.5 rounded-full bg-emerald-500"></span> PAN (Drag)</span>
        }
        <span class="flex items-center gap-1.5"><span class="w-1.5 h-1.5 rounded-full bg-blue-500"></span> ZOOM (Scroll)</span>
      </div>

      <!-- Selected Node Detail Panel -->
      @if (selectedDoc(); as doc) {
        <div class="absolute top-20 left-6 z-30 w-72 bg-zinc-900/95 border border-indigo-500/30 rounded-xl shadow-[0_0_50px_rgba(0,0,0,0.8)] backdrop-blur-xl animate-in slide-in-from-left-4 duration-300 overflow-hidden">
          <div class="h-1 w-full bg-gradient-to-r from-indigo-500 to-violet-500"></div>
          <div class="p-5">
             <div class="flex justify-between items-start mb-2">
                <span class="font-mono text-[10px] text-indigo-400 uppercase tracking-widest bg-indigo-500/10 px-2 py-0.5 rounded border border-indigo-500/20">Vector Node</span>
                <button (click)="clearSelection()" class="text-zinc-500 hover:text-white transition-colors">
                  <svg xmlns="http://www.w3.org/2000/svg" class="h-4 w-4" viewBox="0 0 20 20" fill="currentColor"><path fill-rule="evenodd" d="M4.293 4.293a1 1 0 011.414 0L10 8.586l4.293-4.293a1 1 0 111.414 1.414L11.414 10l4.293 4.293a1 1 0 01-1.414 1.414L10 11.414l-4.293 4.293a1 1 0 01-1.414-1.414L8.586 10 4.293 5.707a1 1 0 010-1.414z" clip-rule="evenodd" /></svg>
                </button>
             </div>
             <h3 class="font-bold text-white text-sm leading-tight mb-3 line-clamp-4">{{ doc.content }}</h3>
             
             <div class="space-y-3">
               <div class="grid grid-cols-2 gap-2">
                  <div class="bg-black/40 p-2 rounded border border-white/5">
                    <div class="text-[9px] text-zinc-500 uppercase">Cluster</div>
                    <div class="text-xs text-zinc-300 font-mono truncate">{{ doc.metadata['category'] || 'Unknown' }}</div>
                  </div>
                  <div class="bg-black/40 p-2 rounded border border-white/5">
                    <div class="text-[9px] text-zinc-500 uppercase">Score</div>
                    <div class="text-xs text-zinc-300 font-mono truncate">0.{{ (doc.id.charCodeAt(0) * 123) % 99 }}91</div>
                  </div>
               </div>
               
               <div class="bg-black/40 p-2 rounded border border-white/5">
                   <div class="text-[9px] text-zinc-500 uppercase mb-1">Coordinates (Latent Space)</div>
                   <div class="flex justify-between font-mono text-[10px] text-emerald-500/80">
                      <span>X: {{ doc.projection3d[0].toFixed(2) }}</span>
                      <span>Y: {{ doc.projection3d[1].toFixed(2) }}</span>
                      <span>Z: {{ doc.projection3d[2].toFixed(2) }}</span>
                   </div>
               </div>

               <div class="pt-2 border-t border-white/5 flex gap-2">
                  <button class="flex-1 bg-indigo-600 hover:bg-indigo-500 text-white text-xs font-medium py-1.5 rounded transition-colors">Query Similar</button>
                  <button class="flex-1 bg-white/5 hover:bg-white/10 text-zinc-300 text-xs font-medium py-1.5 rounded transition-colors">View Raw</button>
               </div>
          </div>
        </div>
      </div>
      }
    </div>
  `
})
export class ProjectionViewComponent implements OnDestroy, AfterViewInit {
  documents = input.required<VectorDoc[]>();
  mode = signal<'2d' | '3d'>('2d');
  selectedDoc = signal<VectorDoc | null>(null);
  
  @ViewChild('chartContainer') container!: ElementRef;
  
  private resizeObserver: ResizeObserver | null = null;
  private colors = ['#818cf8', '#34d399', '#f472b6', '#fbbf24', '#a78bfa']; 
  
  private threeState: {
    scene?: any;
    camera?: any;
    renderer?: any;
    points?: any;
    lines?: any;
    animationId?: number;
    raycaster?: any;
    mouse?: any;
    highlightMesh?: any;
    controls?: {
      isDragging: boolean;
      isPanning: boolean;
      startX: number;
      startY: number;
      lastX: number;
      lastY: number;
      
      // Spherical coords
      targetTheta: number; 
      targetPhi: number;
      targetRadius: number;
      targetLookAt: any; // THREE.Vector3

      // Current for smooth lerping
      currentTheta: number;
      currentPhi: number;
      currentRadius: number;
      currentLookAt: any;
    };
  } = {};

  constructor() {
    effect(() => {
      const docs = this.documents();
      const m = this.mode();
      // Debounce slightly to allow DOM updates
      setTimeout(() => {
        if (this.container && docs.length > 0) {
           this.render();
        }
      }, 50);
    });
  }

  ngAfterViewInit() {
    this.resizeObserver = new ResizeObserver(() => {
       if (this.documents().length) this.render();
    });
    this.resizeObserver.observe(this.container.nativeElement);
  }

  ngOnDestroy() {
    this.resizeObserver?.disconnect();
    this.cleanupThree();
  }

  setMode(m: '2d' | '3d') {
    this.mode.set(m);
  }

  clearSelection() {
    this.selectedDoc.set(null);
    if (this.mode() === '2d') {
      this.render2D();
    }
  }

  private render() {
    const el = this.container.nativeElement;
    el.innerHTML = ''; // Clear previous
    this.cleanupThree();

    if (this.mode() === '2d') {
      this.render2D();
    } else {
      this.render3D();
    }
  }

  private render2D() {
    const data = this.documents();
    const element = this.container.nativeElement;
    const width = element.clientWidth || 800;
    const height = element.clientHeight || 600;

    const svg = d3.select(element)
      .append('svg')
      .attr('width', width)
      .attr('height', height)
      .style('background-color', 'transparent')
      .style('cursor', 'grab');

    // Defs for glow
    const defs = svg.append("defs");
    const filter = defs.append("filter").attr("id", "glow-2d");
    filter.append("feGaussianBlur").attr("stdDeviation", "2.5").attr("result", "coloredBlur");
    const feMerge = filter.append("feMerge");
    feMerge.append("feMergeNode").attr("in", "coloredBlur");
    feMerge.append("feMergeNode").attr("in", "SourceGraphic");

    const g = svg.append('g');

    // Scales
    const xExtent = d3.extent(data, (d: any) => d.projection[0]);
    const yExtent = d3.extent(data, (d: any) => d.projection[1]);
    const xRange = xExtent[1] - xExtent[0];
    const yRange = yExtent[1] - yExtent[0];
    const padding = 0.25;

    const x = d3.scaleLinear()
      .domain([xExtent[0] - xRange * padding, xExtent[1] + xRange * padding])
      .range([-width/2, width/2]);

    const y = d3.scaleLinear()
      .domain([yExtent[0] - yRange * padding, yExtent[1] + yRange * padding])
      .range([-height/2, height/2]);

    const shapeTypes = [d3.symbolCircle, d3.symbolSquare, d3.symbolTriangle, d3.symbolDiamond, d3.symbolCross];

    // Connection Lines
    const links: any[] = [];
    data.forEach((d, i) => {
        const clusterPeers = data.filter((other, j) => i !== j && other.cluster === d.cluster);
        if (clusterPeers.length > 0) {
            const target = clusterPeers[Math.floor(Math.random() * clusterPeers.length)]; 
            if (Math.random() > 0.7) { 
              links.push({ source: d, target: target, color: this.colors[d.cluster! % this.colors.length] });
            }
        }
    });

    g.append('g')
      .selectAll('line')
      .data(links)
      .enter()
      .append('line')
      .attr('x1', (d: any) => x(d.source.projection[0]))
      .attr('y1', (d: any) => y(d.source.projection[1]))
      .attr('x2', (d: any) => x(d.target.projection[0]))
      .attr('y2', (d: any) => y(d.target.projection[1]))
      .attr('stroke', (d: any) => d.color)
      .attr('stroke-width', 0.5)
      .attr('opacity', 0.15);

    // Nodes
    g.append('g')
      .selectAll('path')
      .data(data)
      .enter()
      .append('path')
      .attr('d', d3.symbol()
        .type((d: any) => shapeTypes[d.cluster! % shapeTypes.length])
        .size(100)
      )
      .attr('transform', (d: any) => `translate(${x(d.projection[0])}, ${y(d.projection[1])})`)
      .attr('fill', (d: any) => this.colors[d.cluster! % this.colors.length])
      .attr('stroke', '#09090b')
      .attr('stroke-width', 1.5)
      .style('cursor', 'pointer')
      .style('filter', (d: any) => this.selectedDoc()?.id === d.id ? 'url(#glow-2d)' : 'none')
      .on('mouseover', function(event: any, d: any) {
         d3.select(this)
           .transition().duration(200)
           .attr('d', d3.symbol().type((d: any) => shapeTypes[d.cluster! % shapeTypes.length]).size(250))
           .attr('stroke', '#fff');
      })
      .on('mouseout', (event: any, d: any) => {
         if (this.selectedDoc()?.id !== d.id) {
            d3.select(event.currentTarget)
            .transition().duration(200)
             .attr('d', d3.symbol().type((d: any) => shapeTypes[d.cluster! % shapeTypes.length]).size(100))
            .attr('stroke', '#09090b');
         }
      })
      .on('click', (event: any, d: any) => {
         event.stopPropagation();
         this.selectedDoc.set(d);
         this.render2D(); // Re-render to clear other selections cleanly
      });

    // Zoom
    const zoom = d3.zoom()
      .scaleExtent([0.1, 8])
      .on('zoom', (event: any) => {
        g.attr('transform', event.transform);
        if (event.transform.k > 1) {
            svg.style('cursor', 'grabbing');
        }
      })
      .on('end', () => svg.style('cursor', 'grab'));

    svg.call(zoom)
       .call(zoom.transform, d3.zoomIdentity.translate(width/2, height/2).scale(0.8));

    svg.on('click', () => this.clearSelection());
  }

  private render3D() {
    const data = this.documents();
    const element = this.container.nativeElement;
    const width = element.clientWidth || 800;
    const height = element.clientHeight || 600;

    const scene = new THREE.Scene();
    scene.fog = new THREE.FogExp2(0x050505, 0.002);

    const camera = new THREE.PerspectiveCamera(60, width / height, 0.1, 1000);
    const renderer = new THREE.WebGLRenderer({ alpha: true, antialias: true });
    renderer.setSize(width, height);
    renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
    
    // Crucial: allow pointer events
    renderer.domElement.style.touchAction = 'none';
    renderer.domElement.style.display = 'block';
    element.appendChild(renderer.domElement);

    const gridHelper = new THREE.GridHelper(200, 20, 0x333333, 0x111111);
    gridHelper.position.y = -50;
    scene.add(gridHelper);

    // Texture
    const canvas = document.createElement('canvas');
    canvas.width = 128; 
    canvas.height = 128;
    const ctx = canvas.getContext('2d')!;
    const gradient = ctx.createRadialGradient(64, 64, 0, 64, 64, 64);
    gradient.addColorStop(0, 'rgba(255,255,255,1)');
    gradient.addColorStop(0.2, 'rgba(255,255,255,0.9)');
    gradient.addColorStop(0.5, 'rgba(255,255,255,0.2)');
    gradient.addColorStop(1, 'rgba(0,0,0,0)');
    ctx.fillStyle = gradient;
    ctx.fillRect(0, 0, 128, 128);
    const texture = new THREE.CanvasTexture(canvas);

    // Points
    const geometry = new THREE.BufferGeometry();
    const positions = [];
    const colors = [];
    const colorObj = new THREE.Color();

    for (const doc of data) {
      positions.push(doc.projection3d[0], doc.projection3d[1], doc.projection3d[2]);
      const c = this.colors[doc.cluster! % this.colors.length];
      colorObj.set(c);
      colors.push(colorObj.r, colorObj.g, colorObj.b);
    }

    geometry.setAttribute('position', new THREE.Float32BufferAttribute(positions, 3));
    geometry.setAttribute('color', new THREE.Float32BufferAttribute(colors, 3));

    const material = new THREE.PointsMaterial({
      size: 8,
      map: texture,
      vertexColors: true,
      blending: THREE.AdditiveBlending,
      depthWrite: false,
      transparent: true,
      sizeAttenuation: true
    });

    const points = new THREE.Points(geometry, material);
    scene.add(points);

    // --- 3D Connection Lines ---
    const linePositions: number[] = [];
    const lineColors: number[] = [];
    
    data.forEach((d, i) => {
        const clusterPeers = data.filter((other, j) => i !== j && other.cluster === d.cluster);
        if (clusterPeers.length > 0) {
            // Connect to random peer in cluster
            const target = clusterPeers[Math.floor(Math.random() * clusterPeers.length)]; 
            if (Math.random() > 0.8) { // Very sparse in 3D to avoid clutter
               linePositions.push(
                 d.projection3d[0], d.projection3d[1], d.projection3d[2],
                 target.projection3d[0], target.projection3d[1], target.projection3d[2]
               );
               
               const c = this.colors[d.cluster! % this.colors.length];
               const color = new THREE.Color(c);
               // Push same color twice for solid line vertex gradient
               lineColors.push(color.r, color.g, color.b, color.r, color.g, color.b);
            }
        }
    });

    const lineGeometry = new THREE.BufferGeometry();
    lineGeometry.setAttribute('position', new THREE.Float32BufferAttribute(linePositions, 3));
    lineGeometry.setAttribute('color', new THREE.Float32BufferAttribute(lineColors, 3));

    const lineMaterial = new THREE.LineBasicMaterial({
      vertexColors: true,
      transparent: true,
      opacity: 0.15, // Subtle ghost lines
      blending: THREE.AdditiveBlending,
      depthWrite: false
    });

    const lines = new THREE.LineSegments(lineGeometry, lineMaterial);
    scene.add(lines);

    // Highlight Ring
    const ringGeo = new THREE.RingGeometry(2.5, 3, 32);
    const ringMat = new THREE.MeshBasicMaterial({ color: 0xffffff, side: THREE.DoubleSide, transparent: true, opacity: 0.8 });
    const highlightMesh = new THREE.Mesh(ringGeo, ringMat);
    highlightMesh.visible = false;
    scene.add(highlightMesh);

    // Interaction State
    const raycaster = new THREE.Raycaster();
    raycaster.params.Points.threshold = 4;
    const mouse = new THREE.Vector2(99, 99);

    // Initialize State
    this.threeState = {
      scene, camera, renderer, points, lines, raycaster, mouse, highlightMesh,
      controls: {
        isDragging: false,
        isPanning: false,
        startX: 0,
        startY: 0,
        lastX: 0,
        lastY: 0,
        targetTheta: Math.PI / 4, targetPhi: Math.PI / 3, targetRadius: 150, targetLookAt: new THREE.Vector3(0,0,0),
        currentTheta: Math.PI / 4, currentPhi: Math.PI / 3, currentRadius: 150, currentLookAt: new THREE.Vector3(0,0,0)
      }
    };

    const canvasEl = renderer.domElement;

    // Pointer Events for robust tracking
    const onPointerDown = (e: PointerEvent) => {
      if (!this.threeState.controls) return;
      canvasEl.setPointerCapture(e.pointerId);
      const c = this.threeState.controls;
      c.isDragging = true;
      c.isPanning = e.button === 2 || e.shiftKey; // Right click or Shift+Click
      c.startX = e.clientX;
      c.startY = e.clientY;
      c.lastX = e.clientX;
      c.lastY = e.clientY;
      canvasEl.style.cursor = 'grabbing';
    };

    const onPointerMove = (e: PointerEvent) => {
      e.preventDefault();
      const c = this.threeState.controls;
      if (!c) return;

      const rect = canvasEl.getBoundingClientRect();
      mouse.x = ((e.clientX - rect.left) / width) * 2 - 1;
      mouse.y = -((e.clientY - rect.top) / height) * 2 + 1;

      if (c.isDragging) {
        const deltaX = e.clientX - c.lastX;
        const deltaY = e.clientY - c.lastY;
        c.lastX = e.clientX;
        c.lastY = e.clientY;

        if (c.isPanning) {
           const panSpeed = c.currentRadius * 0.002;
           const forward = new THREE.Vector3();
           camera.getWorldDirection(forward);
           const right = new THREE.Vector3().crossVectors(forward, camera.up).normalize();
           const up = new THREE.Vector3().crossVectors(right, forward).normalize();
           c.targetLookAt.addScaledVector(right, -deltaX * panSpeed);
           c.targetLookAt.addScaledVector(up, deltaY * panSpeed);
        } else {
           c.targetTheta -= deltaX * 0.005;
           c.targetPhi -= deltaY * 0.005;
           // Constrain Phi to avoid flipping
           c.targetPhi = Math.max(0.1, Math.min(Math.PI - 0.1, c.targetPhi));
        }
      }
    };

    const onPointerUp = (e: PointerEvent) => {
       if (!this.threeState.controls) return;
       const c = this.threeState.controls;
       canvasEl.releasePointerCapture(e.pointerId);
       c.isDragging = false;
       canvasEl.style.cursor = 'default';

       // Check if it was a click (little movement)
       const dist = Math.hypot(e.clientX - c.startX, e.clientY - c.startY);
       if (dist < 5) {
          handleSelect();
       }
    };

    const onWheel = (e: WheelEvent) => {
      e.preventDefault();
      if (!this.threeState.controls) return;
      const c = this.threeState.controls;
      c.targetRadius += e.deltaY * 0.1;
      c.targetRadius = Math.max(10, Math.min(600, c.targetRadius));
    };

    const handleSelect = () => {
      raycaster.setFromCamera(mouse, camera);
      const intersects = raycaster.intersectObject(points);
      
      if (intersects.length > 0) {
        const index = intersects[0].index;
        const doc = data[index!];
        this.selectedDoc.set(doc);
        const p = intersects[0].point;
        highlightMesh.position.copy(p);
        highlightMesh.visible = true;
      } else {
        this.selectedDoc.set(null);
        highlightMesh.visible = false;
      }
    };

    canvasEl.addEventListener('pointerdown', onPointerDown);
    canvasEl.addEventListener('pointermove', onPointerMove);
    canvasEl.addEventListener('pointerup', onPointerUp);
    canvasEl.addEventListener('wheel', onWheel, { passive: false });
    canvasEl.addEventListener('contextmenu', (e: Event) => e.preventDefault());

    canvasEl._cleanup = () => {
       canvasEl.removeEventListener('pointerdown', onPointerDown);
       canvasEl.removeEventListener('pointermove', onPointerMove);
       canvasEl.removeEventListener('pointerup', onPointerUp);
       canvasEl.removeEventListener('wheel', onWheel);
       canvasEl.removeEventListener('contextmenu', (e: Event) => e.preventDefault());
    };

    const animate = () => {
      this.threeState.animationId = requestAnimationFrame(animate);
      if (!this.threeState.controls || !this.threeState.renderer) return;

      const c = this.threeState.controls;
      
      // Auto-rotation idle
      if (!c.isDragging && !this.selectedDoc()) c.targetTheta += 0.0005;

      const damping = 0.1;
      c.currentTheta += (c.targetTheta - c.currentTheta) * damping;
      c.currentPhi += (c.targetPhi - c.currentPhi) * damping;
      c.currentRadius += (c.targetRadius - c.currentRadius) * damping;
      c.currentLookAt.lerp(c.targetLookAt, damping);

      const x = c.currentRadius * Math.sin(c.currentPhi) * Math.sin(c.currentTheta);
      const y = c.currentRadius * Math.cos(c.currentPhi);
      const z = c.currentRadius * Math.sin(c.currentPhi) * Math.cos(c.currentTheta);

      camera.position.set(x + c.currentLookAt.x, y + c.currentLookAt.y, z + c.currentLookAt.z);
      camera.lookAt(c.currentLookAt);

      // Highlight logic
      if (highlightMesh.visible) {
         highlightMesh.lookAt(camera.position);
         const scale = 1 + Math.sin(Date.now() * 0.005) * 0.1;
         highlightMesh.scale.set(scale, scale, 1);
      }
      
      // Update cursor for hover
      raycaster.setFromCamera(mouse, camera);
      const intersects = raycaster.intersectObject(points);
      if (!c.isDragging) {
         canvasEl.style.cursor = intersects.length > 0 ? 'pointer' : 'default';
      }

      renderer.render(scene, camera);
    };

    animate();
  }

  private cleanupThree() {
    if (this.threeState.animationId) cancelAnimationFrame(this.threeState.animationId);
    if (this.threeState.renderer) {
      if (this.threeState.renderer.domElement._cleanup) this.threeState.renderer.domElement._cleanup();
      this.threeState.renderer.dispose();
      if (this.threeState.renderer.domElement.parentNode) {
         this.threeState.renderer.domElement.parentNode.removeChild(this.threeState.renderer.domElement);
      }
    }
    if (this.threeState.points) {
       this.threeState.points.geometry.dispose();
       this.threeState.points.material.dispose();
    }
    if (this.threeState.lines) {
        this.threeState.lines.geometry.dispose();
        this.threeState.lines.material.dispose();
    }
    this.threeState = {};
  }
}