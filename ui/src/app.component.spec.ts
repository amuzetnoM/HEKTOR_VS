import { TestBed, ComponentFixture } from '@angular/core/testing';
import { HttpClientTestingModule } from '@angular/common/http/testing';
import { AppComponent } from './app.component';
import { VectorDbService } from './services/vector-db.service';
import { AgentService } from './services/agent.service';

describe('AppComponent', () => {
    let component: AppComponent;
    let fixture: ComponentFixture<AppComponent>;
    let vectorDbService: VectorDbService;

    beforeEach(async () => {
        await TestBed.configureTestingModule({
            imports: [HttpClientTestingModule, AppComponent],
            providers: [VectorDbService, AgentService]
        }).compileComponents();

        fixture = TestBed.createComponent(AppComponent);
        component = fixture.componentInstance;
        vectorDbService = TestBed.inject(VectorDbService);
        fixture.detectChanges();
    });

    it('should create the app component', () => {
        expect(component).toBeTruthy();
    });

    it('should have collections signal', () => {
        expect(component.collections).toBeDefined();
    });

    it('should have stats computed signal', () => {
        expect(component.stats).toBeDefined();
        const stats = component.stats();
        expect(stats).toHaveProperty('latency');
        expect(stats).toHaveProperty('memoryUsage');
        expect(stats).toHaveProperty('totalDocs');
    });

    it('should initialize with collections from service', () => {
        const collections = component.collections();
        expect(Array.isArray(collections)).toBe(true);
    });

    it('should handle collection selection', () => {
        // Setup test data
        vectorDbService.collections.set([
            {
                name: 'test-collection',
                dimension: 1536,
                metric: 'cosine',
                documents: [],
                schema: []
            }
        ]);

        // Test collection selection logic
        const collections = component.collections();
        expect(collections.length).toBeGreaterThan(0);
    });
});
