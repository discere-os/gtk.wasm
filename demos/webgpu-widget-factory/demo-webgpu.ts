// WebGPU GTK4 Widget Factory Demo
// MAIN module hosting SIDE modules: gtk.wasm, glib.wasm, cairo.wasm, pango.wasm

export interface WebGPUWidgetFactoryDemo {
  initialize(canvas: HTMLCanvasElement): Promise<void>;
  render(): Promise<void>;
  handleResize(width: number, height: number): void;
  getPerformanceMetrics(): Promise<PerformanceMetrics>;
  startStressTest(widgetCount: number): Promise<void>;
  cleanup(): void;
}

export interface PerformanceMetrics {
  fps: number;
  frameTime: number;
  drawCalls: number;
  triangles: number;
  memoryUsage: number;
  simdSpeedup: number;
}

export default class WebGPUWidgetFactoryDemoImpl implements WebGPUWidgetFactoryDemo {
  private module: any = null;
  private initialized = false;
  private canvas: HTMLCanvasElement | null = null;
  private device: GPUDevice | null = null;
  private context: GPUCanvasContext | null = null;

  // Side modules (loaded dynamically)
  private sideModules = new Map<string, any>();

  // Performance tracking
  private frameCount = 0;
  private lastFrameTime = 0;
  private performanceStartTime = 0;

  // Widget state
  private widgets = new Map<string, any>();
  private renderNodes: any[] = [];

  async initialize(canvas: HTMLCanvasElement): Promise<void> {
    if (this.initialized) return;

    // WebGPU is mandatory - fail fast if unavailable
    if (!navigator.gpu) {
      throw new Error('WebGPU is required. Please use Chrome/Edge 113+ with WebGPU enabled.');
    }

    this.canvas = canvas;

    try {
      // Request WebGPU adapter with high performance
      const adapter = await navigator.gpu.requestAdapter({
        powerPreference: "high-performance"
      });

      if (!adapter) {
        throw new Error('Failed to get WebGPU adapter');
      }

      // Request device with advanced features
      this.device = await adapter.requestDevice({
        requiredFeatures: [
          "timestamp-query",
          "texture-compression-bc"
        ].filter(feature => adapter.features.has(feature as GPUFeatureName)) as GPUFeatureName[]
      });

      // Configure canvas context
      this.context = canvas.getContext('webgpu')!;
      const swapChainFormat = navigator.gpu.getPreferredCanvasFormat();

      this.context.configure({
        device: this.device,
        format: swapChainFormat,
        alphaMode: 'premultiplied'
      });

      // Load main WASM module (GTK with WebGPU renderer)
      await this.loadMainModule();

      // Load SIDE modules
      await this.loadSideModules();

      // Initialize GTK with WebGPU backend
      await this.initializeGTK();

      // Create widget factory demo content
      await this.createWidgetFactory();

      this.initialized = true;
      this.performanceStartTime = performance.now();

      console.log('WebGPU Widget Factory Demo initialized successfully');

    } catch (error) {
      throw new Error(`Failed to initialize WebGPU Widget Factory Demo: ${error}`);
    }
  }

  private async loadMainModule(): Promise<void> {
    try {
      // Try local development first (Deno)
      if (typeof globalThis.Deno !== 'undefined') {
        const moduleFactory = (await import('../../install/wasm/gtk-webgpu-main.js')).default;
        const wasmBinary = await this.loadLocalWasmBinary('../../install/wasm/gtk-webgpu-main.wasm');

        this.module = await moduleFactory({
          wasmBinary,
          webgpuDevice: this.device,
          canvas: this.canvas
        });
        return;
      }

      // Try CDN locations
      const cdnUrls = [
        'https://wasm.discere.cloud/gtk/latest/main/',
        'https://cdn.jsdelivr.net/npm/@discere-os/gtk.wasm/dist/'
      ];

      for (const url of cdnUrls) {
        try {
          const moduleFactory = (await import(`${url}gtk-webgpu-main.js`)).default;
          const wasmBinary = await this.loadCDNWasmBinary(`${url}gtk-webgpu-main.wasm`);

          this.module = await moduleFactory({
            wasmBinary,
            webgpuDevice: this.device,
            canvas: this.canvas
          });
          return;
        } catch { continue; }
      }

      throw new Error('Failed to load main module from any source');
    } catch (error) {
      throw new Error(`Failed to load main WASM module: ${error}`);
    }
  }

  private async loadLocalWasmBinary(path: string): Promise<ArrayBuffer | undefined> {
    try {
      const wasmPath = new URL(path, import.meta.url).pathname;
      const wasmBuffer = await Deno.readFile(wasmPath);
      return wasmBuffer.buffer;
    } catch {
      return undefined;
    }
  }

  private async loadCDNWasmBinary(url: string): Promise<ArrayBuffer | undefined> {
    try {
      const response = await fetch(url);
      if (response.ok) {
        return await response.arrayBuffer();
      }
    } catch { }
    return undefined;
  }

  private async loadSideModules(): Promise<void> {
    const sideModules = [
      { name: 'glib', url: '../glib.wasm/install/wasm/glib-side.wasm' },
      { name: 'cairo', url: '../cairo.wasm/install/wasm/cairo-side.wasm' },
      { name: 'pango', url: '../pango.wasm/install/wasm/pango-side.wasm' },
      { name: 'pixman', url: '../pixman.wasm/install/wasm/pixman-side.wasm' },
      { name: 'harfbuzz', url: '../harfbuzz.wasm/install/wasm/harfbuzz-side.wasm' },
      { name: 'fontconfig', url: '../fontconfig.wasm/install/wasm/fontconfig-side.wasm' },
      { name: 'freetype', url: '../freetype.wasm/install/wasm/freetype-side.wasm' }
    ];

    for (const sideModule of sideModules) {
      try {
        // Load SIDE module using dlopen() pattern
        const handle = this.module._dlopen(sideModule.url, 1); // RTLD_NOW
        if (handle) {
          this.sideModules.set(sideModule.name, handle);
          console.log(`Loaded SIDE module: ${sideModule.name}`);
        } else {
          console.warn(`Failed to load SIDE module: ${sideModule.name}`);
        }
      } catch (error) {
        console.warn(`Error loading SIDE module ${sideModule.name}:`, error);
      }
    }

    console.log(`Loaded ${this.sideModules.size} SIDE modules:`, Array.from(this.sideModules.keys()));
  }

  private async initializeGTK(): Promise<void> {
    // Initialize GTK with WebGPU renderer
    const result = this.module._gtk_init_webgpu(this.device, this.context);
    if (result !== 0) {
      throw new Error(`GTK WebGPU initialization failed: ${result}`);
    }

    // Enable SIMD optimizations if available
    if (this.module._gsk_enable_simd_optimizations) {
      const simdEnabled = this.module._gsk_enable_simd_optimizations();
      console.log(`SIMD optimizations: ${simdEnabled ? 'enabled' : 'disabled'}`);
    }

    // Set up performance monitoring
    if (this.module._gsk_webgpu_enable_performance_monitoring) {
      this.module._gsk_webgpu_enable_performance_monitoring(true);
    }
  }

  private async createWidgetFactory(): Promise<void> {
    // Create main window
    const window = this.module._gtk_window_new();
    this.module._gtk_window_set_title(window, "WebGPU Widget Factory");
    this.module._gtk_window_set_default_size(window, 800, 600);

    this.widgets.set('main-window', window);

    // Create notebook for organizing widgets
    const notebook = this.module._gtk_notebook_new();
    this.module._gtk_window_set_child(window, notebook);
    this.widgets.set('notebook', notebook);

    // Create widget pages
    await this.createButtonsPage(notebook);
    await this.createEntryPage(notebook);
    await this.createListPage(notebook);
    await this.createProgressPage(notebook);
    await this.createImagesPage(notebook);

    // Show all widgets
    this.module._gtk_widget_show(window);

    console.log('Widget factory created with', this.widgets.size, 'widgets');
  }

  private async createButtonsPage(notebook: any): Promise<void> {
    const grid = this.module._gtk_grid_new();
    this.module._gtk_grid_set_row_spacing(grid, 6);
    this.module._gtk_grid_set_column_spacing(grid, 6);

    // Various button types
    const buttons = [
      { text: "Normal Button", row: 0, col: 0 },
      { text: "Toggle Button", row: 0, col: 1, toggle: true },
      { text: "Check Button", row: 1, col: 0, check: true },
      { text: "Radio Button", row: 1, col: 1, radio: true },
      { text: "Link Button", row: 2, col: 0, link: true },
      { text: "Menu Button", row: 2, col: 1, menu: true }
    ];

    for (const buttonConfig of buttons) {
      let button;

      if (buttonConfig.toggle) {
        button = this.module._gtk_toggle_button_new_with_label(buttonConfig.text);
      } else if (buttonConfig.check) {
        button = this.module._gtk_check_button_new_with_label(buttonConfig.text);
      } else if (buttonConfig.radio) {
        button = this.module._gtk_radio_button_new_with_label(null, buttonConfig.text);
      } else if (buttonConfig.link) {
        button = this.module._gtk_link_button_new_with_label("https://gtk.org", buttonConfig.text);
      } else if (buttonConfig.menu) {
        button = this.module._gtk_menu_button_new();
        this.module._gtk_button_set_label(button, buttonConfig.text);
      } else {
        button = this.module._gtk_button_new_with_label(buttonConfig.text);
      }

      this.module._gtk_grid_attach(grid, button, buttonConfig.col, buttonConfig.row, 1, 1);
      this.widgets.set(`button-${buttonConfig.row}-${buttonConfig.col}`, button);
    }

    // Add tab to notebook
    const label = this.module._gtk_label_new("Buttons");
    this.module._gtk_notebook_append_page(notebook, grid, label);
    this.widgets.set('buttons-page', grid);
  }

  private async createEntryPage(notebook: any): Promise<void> {
    const box = this.module._gtk_box_new(1, 6); // GTK_ORIENTATION_VERTICAL = 1

    // Text entry
    const entry = this.module._gtk_entry_new();
    this.module._gtk_entry_set_placeholder_text(entry, "Enter text here...");
    this.module._gtk_box_append(box, entry);
    this.widgets.set('text-entry', entry);

    // Password entry
    const password = this.module._gtk_entry_new();
    this.module._gtk_entry_set_placeholder_text(password, "Password");
    this.module._gtk_entry_set_visibility(password, false);
    this.module._gtk_box_append(box, password);
    this.widgets.set('password-entry', password);

    // Search entry
    const search = this.module._gtk_search_entry_new();
    this.module._gtk_box_append(box, search);
    this.widgets.set('search-entry', search);

    // Text view
    const textView = this.module._gtk_text_view_new();
    const buffer = this.module._gtk_text_view_get_buffer(textView);
    this.module._gtk_text_buffer_set_text(buffer, "WebGPU accelerated text rendering\nwith SIMD optimizations", -1);

    const scrolled = this.module._gtk_scrolled_window_new();
    this.module._gtk_scrolled_window_set_child(scrolled, textView);
    this.module._gtk_scrolled_window_set_policy(scrolled, 1, 1); // GTK_POLICY_AUTOMATIC
    this.module._gtk_box_append(box, scrolled);
    this.widgets.set('text-view', textView);

    // Add tab to notebook
    const label = this.module._gtk_label_new("Text Input");
    this.module._gtk_notebook_append_page(notebook, box, label);
    this.widgets.set('entry-page', box);
  }

  private async createListPage(notebook: any): Promise<void> {
    const box = this.module._gtk_box_new(1, 6);

    // List box
    const listBox = this.module._gtk_list_box_new();

    // Add list items
    for (let i = 0; i < 20; i++) {
      const row = this.module._gtk_list_box_row_new();
      const label = this.module._gtk_label_new(`List Item ${i + 1} - WebGPU Rendered`);
      this.module._gtk_list_box_row_set_child(row, label);
      this.module._gtk_list_box_append(listBox, row);
      this.widgets.set(`list-item-${i}`, row);
    }

    const scrolled = this.module._gtk_scrolled_window_new();
    this.module._gtk_scrolled_window_set_child(scrolled, listBox);
    this.module._gtk_scrolled_window_set_policy(scrolled, 0, 1); // Never horizontal, automatic vertical
    this.module._gtk_box_append(box, scrolled);

    this.widgets.set('list-box', listBox);

    // Add tab to notebook
    const label = this.module._gtk_label_new("Lists");
    this.module._gtk_notebook_append_page(notebook, box, label);
    this.widgets.set('list-page', box);
  }

  private async createProgressPage(notebook: any): Promise<void> {
    const grid = this.module._gtk_grid_new();
    this.module._gtk_grid_set_row_spacing(grid, 12);
    this.module._gtk_grid_set_column_spacing(grid, 12);

    // Progress bar
    const progressBar = this.module._gtk_progress_bar_new();
    this.module._gtk_progress_bar_set_fraction(progressBar, 0.65);
    this.module._gtk_progress_bar_set_text(progressBar, "65% Complete");
    this.module._gtk_progress_bar_set_show_text(progressBar, true);
    this.module._gtk_grid_attach(grid, progressBar, 0, 0, 2, 1);
    this.widgets.set('progress-bar', progressBar);

    // Level bar
    const levelBar = this.module._gtk_level_bar_new();
    this.module._gtk_level_bar_set_value(levelBar, 0.75);
    this.module._gtk_grid_attach(grid, levelBar, 0, 1, 2, 1);
    this.widgets.set('level-bar', levelBar);

    // Scale (slider)
    const scale = this.module._gtk_scale_new_with_range(0, 0, 100, 1);
    this.module._gtk_range_set_value(scale, 50);
    this.module._gtk_scale_set_draw_value(scale, true);
    this.module._gtk_grid_attach(grid, scale, 0, 2, 2, 1);
    this.widgets.set('scale', scale);

    // Spinner
    const spinner = this.module._gtk_spinner_new();
    this.module._gtk_spinner_start(spinner);
    this.module._gtk_grid_attach(grid, spinner, 0, 3, 1, 1);
    this.widgets.set('spinner', spinner);

    // Add tab to notebook
    const label = this.module._gtk_label_new("Progress");
    this.module._gtk_notebook_append_page(notebook, grid, label);
    this.widgets.set('progress-page', grid);
  }

  private async createImagesPage(notebook: any): Promise<void> {
    const grid = this.module._gtk_grid_new();
    this.module._gtk_grid_set_row_spacing(grid, 6);
    this.module._gtk_grid_set_column_spacing(grid, 6);

    // Create some test images/icons
    const iconNames = ['document-new', 'document-open', 'document-save', 'edit-copy', 'edit-paste', 'edit-undo'];

    for (let i = 0; i < iconNames.length; i++) {
      const row = Math.floor(i / 3);
      const col = i % 3;

      const image = this.module._gtk_image_new_from_icon_name(iconNames[i]);
      this.module._gtk_image_set_pixel_size(image, 48);
      this.module._gtk_grid_attach(grid, image, col, row, 1, 1);
      this.widgets.set(`image-${iconNames[i]}`, image);
    }

    // Add tab to notebook
    const label = this.module._gtk_label_new("Images");
    this.module._gtk_notebook_append_page(notebook, grid, label);
    this.widgets.set('images-page', grid);
  }

  async render(): Promise<void> {
    if (!this.initialized || !this.module || !this.device || !this.context) {
      return;
    }

    const frameStartTime = performance.now();

    try {
      // Update any animated widgets first
      this.updateAnimatedWidgets();

      // Get current swap chain texture for WebGPU
      const currentTexture = this.context.getCurrentTexture();
      const textureView = currentTexture.createView();

      // Create command encoder for this frame
      const commandEncoder = this.device.createCommandEncoder({
        label: 'GTK WebGPU Render Frame'
      });

      // Begin render pass with GTK's background color
      const renderPassEncoder = commandEncoder.beginRenderPass({
        label: 'GTK Main Render Pass',
        colorAttachments: [{
          view: textureView,
          clearValue: { r: 0.95, g: 0.95, b: 0.95, a: 1.0 }, // Light gray
          loadOp: 'clear',
          storeOp: 'store'
        }]
      });

      // Call GTK WebGPU renderer with native WebGPU context
      if (this.module._gtk_webgpu_render_frame_with_encoder) {
        const renderStats = this.module._gtk_webgpu_render_frame_with_encoder(
          renderPassEncoder, // Pass WebGPU render pass encoder
          this.widgets.size,  // Number of widgets to render
          this.canvas!.width,
          this.canvas!.height,
          frameStartTime
        );

        // Update render node information if available
        if (renderStats && renderStats.renderNodeCount) {
          this.renderNodes = new Array(renderStats.renderNodeCount);
        }
      } else {
        // Fallback to basic render call
        const result = this.module._gtk_webgpu_render_frame();
        if (result !== 0) {
          console.warn(`Render frame returned: ${result}`);
        }
      }

      // End render pass and submit commands
      renderPassEncoder.end();
      const commandBuffer = commandEncoder.finish();
      this.device.queue.submit([commandBuffer]);

      // Apply SIMD post-processing optimizations if available
      if (this.module._gsk_simd_post_process_frame) {
        this.module._gsk_simd_post_process_frame(
          this.canvas!.width,
          this.canvas!.height
        );
      }

      // Update performance counters
      this.frameCount++;
      this.lastFrameTime = performance.now() - frameStartTime;

    } catch (error) {
      console.error('WebGPU render error:', error);

      // Attempt graceful fallback
      if (this.module._gtk_webgpu_render_frame_fallback) {
        try {
          this.module._gtk_webgpu_render_frame_fallback();
        } catch (fallbackError) {
          console.error('Fallback render also failed:', fallbackError);
        }
      }
    }
  }

  private updateAnimatedWidgets(): void {
    // Animate progress bar
    const progressBar = this.widgets.get('progress-bar');
    if (progressBar) {
      const time = (performance.now() / 2000) % 1; // 2 second cycle
      const progress = (Math.sin(time * Math.PI * 2) + 1) / 2; // 0-1 sine wave
      this.module._gtk_progress_bar_set_fraction(progressBar, progress);
      this.module._gtk_progress_bar_set_text(progressBar, `${Math.round(progress * 100)}% Complete`);
    }

    // Animate scale
    const scale = this.widgets.get('scale');
    if (scale) {
      const time = (performance.now() / 3000) % 1; // 3 second cycle
      const value = Math.sin(time * Math.PI * 2) * 40 + 50; // 10-90 range
      this.module._gtk_range_set_value(scale, value);
    }
  }

  handleResize(width: number, height: number): void {
    if (!this.initialized || !this.canvas) {
      return;
    }

    this.canvas.width = width;
    this.canvas.height = height;

    // Notify GTK of size change
    if (this.module._gtk_webgpu_resize) {
      this.module._gtk_webgpu_resize(width, height);
    }

    // Update main window size
    const window = this.widgets.get('main-window');
    if (window) {
      this.module._gtk_window_set_default_size(window, width, height);
    }
  }

  async getPerformanceMetrics(): Promise<PerformanceMetrics> {
    if (!this.initialized || !this.module) {
      return {
        fps: 0,
        frameTime: 0,
        drawCalls: 0,
        triangles: 0,
        memoryUsage: 0,
        simdSpeedup: 1.0
      };
    }

    const currentTime = performance.now();
    const elapsed = currentTime - this.performanceStartTime;
    const fps = this.frameCount / (elapsed / 1000);

    let metrics: PerformanceMetrics = {
      fps: fps,
      frameTime: this.lastFrameTime - (this.lastFrameTime - 16.67), // Approximate
      drawCalls: 0,
      triangles: 0,
      memoryUsage: 0,
      simdSpeedup: 1.0
    };

    // Get detailed metrics from WASM module
    if (this.module._gsk_webgpu_get_performance_metrics) {
      const wasmMetrics = this.module._gsk_webgpu_get_performance_metrics();
      if (wasmMetrics) {
        metrics.drawCalls = wasmMetrics.drawCalls || 0;
        metrics.triangles = wasmMetrics.triangles || 0;
        metrics.memoryUsage = wasmMetrics.memoryUsage || 0;
        metrics.simdSpeedup = wasmMetrics.simdSpeedup || 1.0;
      }
    }

    return metrics;
  }

  async startStressTest(widgetCount: number): Promise<void> {
    console.log(`Starting stress test with ${widgetCount} widgets...`);

    // Reset performance counters
    this.frameCount = 0;
    this.performanceStartTime = performance.now();

    // Create additional widgets for stress testing
    const testContainer = this.module._gtk_box_new(1, 2); // Vertical box

    for (let i = 0; i < widgetCount; i++) {
      const button = this.module._gtk_button_new_with_label(`Stress Test Button ${i + 1}`);
      this.module._gtk_box_append(testContainer, button);
      this.widgets.set(`stress-button-${i}`, button);
    }

    // Add to a new notebook page
    const notebook = this.widgets.get('notebook');
    if (notebook) {
      const scrolled = this.module._gtk_scrolled_window_new();
      this.module._gtk_scrolled_window_set_child(scrolled, testContainer);
      this.module._gtk_scrolled_window_set_policy(scrolled, 0, 1);

      const label = this.module._gtk_label_new(`Stress Test (${widgetCount})`);
      this.module._gtk_notebook_append_page(notebook, scrolled, label);
      this.widgets.set('stress-test-page', scrolled);
    }

    console.log(`Stress test setup complete. Total widgets: ${this.widgets.size}`);
  }

  cleanup(): void {
    // Clean up widgets
    this.widgets.clear();
    this.renderNodes = [];

    // Clean up SIDE modules
    for (const [name, handle] of this.sideModules) {
      if (this.module._dlclose) {
        this.module._dlclose(handle);
      }
    }
    this.sideModules.clear();

    // Clean up main module
    if (this.module) {
      if (this.module._gtk_webgpu_cleanup) {
        this.module._gtk_webgpu_cleanup();
      }
      this.module = null;
    }

    // Clean up WebGPU resources
    this.device = null;
    this.context = null;
    this.canvas = null;

    this.initialized = false;
    console.log('WebGPU Widget Factory Demo cleaned up');
  }
}