#include "GPUProfiler.h"
#include "GlobalDX.h"
#include "CommandQueue.h"

#include <implot.h>
#include <implot_internal.h>

#include "Core/String.h"

namespace RT
{

	static constexpr size_t DEFAULT_MAX_SCROLLING_BUFFER_SIZE = 1000;

	const char* GetGPUTimerLabel(GPUProfiler::GPUTimer timer)
	{
		switch (timer)
		{
		case GPUProfiler::GPUTimer_FrameTime:
			return "Frame time";
		case GPUProfiler::GPUTimer_BuildTLAS:
			return "Build TLAS";
		case GPUProfiler::GPUTimer_PrimaryRay:
			return "Primary ray";
		case GPUProfiler::GPUTimer_DirectLighting:
			return "Direct lighting";
		case GPUProfiler::GPUTimer_IndirectLighting:
			return "Indirect lighting";
		case GPUProfiler::GPUTimer_DenoiseResample:
			return "Denoise resample";
		case GPUProfiler::GPUTimer_DenoiseSVGF:
			return "Denoise SVGF";
		case GPUProfiler::GPUTimer_Composite:
			return "Composite";
		case GPUProfiler::GPUTimer_TAA:
			return "TAA";
		case GPUProfiler::GPUTimer_Bloom:
			return "Bloom";
		case GPUProfiler::GPUTimer_PostProcess:
			return "Post-process";
		default:
			return "Unknown";
		}
	}

	template <typename T>
	struct GraphDataNode
	{
		static_assert(std::is_arithmetic<T>::value, "GraphDataNode is only allowed to be used with arithmetic data types");

		float t;
		T value;
	};

	template <typename T>
	struct GraphScrollingBuffer
	{
		size_t max_size;
		size_t current_size;
		size_t current_offset;
		GraphDataNode<T>* data_buf;

		void AddDataNode(float t, T value)
		{
			if (current_size < max_size)
			{
				data_buf[current_size] = { t, value };
				current_size++;
			}
			else
			{
				data_buf[current_offset] = { t, value };
				current_offset = (current_offset + 1) % max_size;
			}
		}
	};

	GraphScrollingBuffer<uint32_t> g_back_buffer_scrolling_buffer;
	GraphScrollingBuffer<uint64_t> g_frame_index_scrolling_buffer;
	GraphScrollingBuffer<float> g_scrolling_buffers[GPUProfiler::GPUTimer_NumTimers];
	GPUProfiler::GPUTimestamp g_timestamp_queries[BACK_BUFFER_COUNT][GPUProfiler::GPUTimer_NumTimers];

	float g_time_passed = 0.0f, g_graph_history_timespan = 10.0f;
	bool g_pause_graph_scroll = false;

	template <typename T>
	void InitGraphScrollingBuffer(GraphScrollingBuffer<T>& buf, size_t size)
	{
		buf.max_size = size;
		buf.current_size = 0;
		buf.current_offset = 0;
		buf.data_buf = RT_ArenaAllocArray(g_d3d.arena, buf.max_size, GraphDataNode<T>);
	}

	template <typename T>
	GraphDataNode<T> FindNearestDataNode(GraphScrollingBuffer<T>& buffer, int start, int count, float nearest_to, size_t nearest_idx = 0) {
		if (count > 0)
		{
			int half_count = count / 2;
			int mid = (start + half_count) % (int)buffer.max_size;
			float mid_value = buffer.data_buf[mid].t;

			if (mid_value > nearest_to)
				return FindNearestDataNode<T>(buffer, start, half_count - 1, nearest_to, nearest_idx);

			nearest_idx = (size_t)mid;
			return FindNearestDataNode<T>(buffer, mid + 1, half_count, nearest_to, nearest_idx);
		}

		return buffer.data_buf[nearest_idx];
	}

	void GPUProfiler::Init()
	{
		InitGraphScrollingBuffer(g_back_buffer_scrolling_buffer, DEFAULT_MAX_SCROLLING_BUFFER_SIZE);
		InitGraphScrollingBuffer(g_frame_index_scrolling_buffer, DEFAULT_MAX_SCROLLING_BUFFER_SIZE);

		for (size_t i = 0; i < GPUProfiler::GPUTimer_NumTimers; ++i)
			InitGraphScrollingBuffer(g_scrolling_buffers[i], DEFAULT_MAX_SCROLLING_BUFFER_SIZE);

		ImPlot::CreateContext();
	}

	void GPUProfiler::Exit()
	{
		ImPlot::DestroyContext();
	}

	void GPUProfiler::BeginTimestampQuery(ID3D12GraphicsCommandList4* command_list, GPUTimer timer)
	{
		uint32_t timestamp_index = (uint32_t)(g_d3d.current_back_buffer_index * GPUProfiler::GPUTimer_NumTimers * 2 + timer * 2);
		command_list->EndQuery(g_d3d.query_heap, D3D12_QUERY_TYPE_TIMESTAMP, timestamp_index);
		g_timestamp_queries[g_d3d.current_back_buffer_index][timer].StartIndex = timestamp_index;
	}

	void GPUProfiler::EndTimestampQuery(ID3D12GraphicsCommandList4* command_list, GPUTimer timer)
	{
		command_list->EndQuery(g_d3d.query_heap, D3D12_QUERY_TYPE_TIMESTAMP, g_d3d.current_back_buffer_index * GPUProfiler::GPUTimer_NumTimers * 2 + timer * 2 + 1);
	}

	void GPUProfiler::ResolveTimestampQueries(ID3D12GraphicsCommandList4* command_list)
	{
		command_list->ResolveQueryData(g_d3d.query_heap, D3D12_QUERY_TYPE_TIMESTAMP, g_d3d.current_back_buffer_index * GPUProfiler::GPUTimer_NumTimers * 2,
			GPUProfiler::GPUTimer_NumTimers * 2, g_d3d.query_readback_buffer, RT_ALIGN_POW2(g_d3d.current_back_buffer_index * GPUProfiler::GPUTimer_NumTimers * 2 * sizeof(uint64_t), sizeof(uint64_t)));
	}

	void GPUProfiler::ReadbackTimestampsFromBuffer()
	{
		uint64_t start_query_index = g_d3d.current_back_buffer_index * GPUProfiler::GPUTimer_NumTimers * 2;

		D3D12_RANGE read_range = {};
		read_range.Begin = sizeof(uint64_t) * start_query_index;
		read_range.End = read_range.Begin + sizeof(uint64_t) * GPUProfiler::GPUTimer_NumTimers * 2;

		uint64_t* readback_buffer_base;
		g_d3d.query_readback_buffer->Map(0, &read_range, reinterpret_cast<void**>(&readback_buffer_base));

		for (size_t i = 0; i < GPUProfiler::GPUTimer_NumTimers; ++i)
		{
			GPUProfiler::GPUTimestamp& query = g_timestamp_queries[g_d3d.current_back_buffer_index][i];

			query.BeginTimestamp = readback_buffer_base[query.StartIndex + 0];
			query.EndTimestamp = readback_buffer_base[query.StartIndex + 1];
		}

		D3D12_RANGE null_range = { 0, 0 };
		g_d3d.query_readback_buffer->Unmap(0, &null_range);
	}

	void GPUProfiler::RenderImGuiMenus()
	{
		// This will read the timestamp queries from the last time this back buffer/frame has been used, since we can only use timestamps
		// we know have finished executing alongside the rest of the command list (has been waited on in SwapBuffers).
		// This means that timers shown in the current frame are actually BACK_BUFFER_COUNT frames old.
		if (!g_pause_graph_scroll)
		{
			GPUProfiler::ReadbackTimestampsFromBuffer();
			g_time_passed += ImGui::GetIO().DeltaTime;

			uint64_t queue_timestamp_frequency = 0;
			g_d3d.command_queue_direct->GetD3D12CommandQueue()->GetTimestampFrequency(&queue_timestamp_frequency);

			// Add data points for each timer to their respective scrolling buffers
			for (size_t i = 0; i < GPUProfiler::GPUTimer_NumTimers; ++i)
			{
				const GPUProfiler::GPUTimestamp& query = g_timestamp_queries[g_d3d.current_back_buffer_index][i];
				double timer_in_ms = (query.EndTimestamp - query.BeginTimestamp) / (double)queue_timestamp_frequency * 1000.0;
				g_scrolling_buffers[i].AddDataNode(g_time_passed, (float)timer_in_ms);
			}

			// Add data point for current back buffer
			g_back_buffer_scrolling_buffer.AddDataNode(g_time_passed, g_d3d.current_back_buffer_index);
			g_frame_index_scrolling_buffer.AddDataNode(g_time_passed, g_d3d.frame_index);
		}

		if (ImGui::Begin("GPU Profiler"))
		{
			ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			if (ImGui::CollapsingHeader("GPU Timers"))
			{
				ImGui::Indent(8.0f);

				for (size_t i = 0; i < GPUProfiler::GPUTimer_NumTimers; ++i)
				{
					size_t prev_offset = (g_scrolling_buffers[i].current_size + g_scrolling_buffers[i].current_offset + (DEFAULT_MAX_SCROLLING_BUFFER_SIZE - 1)) % DEFAULT_MAX_SCROLLING_BUFFER_SIZE;
					ImGui::Text("%s: %.3f ms", GetGPUTimerLabel((GPUTimer)i), g_scrolling_buffers[i].data_buf[prev_offset].value);
				}

				// Plot GPU timers onto a realtime graph
				ImGui::DragFloat("Graph history timespan", &g_graph_history_timespan, 0.1f, 1.0f, 100.0f, "%.1f s");
				ImGui::Checkbox("Pause graph scroll", &g_pause_graph_scroll);

				ImPlot::SetNextAxisToFit(ImAxis_Y1);
				if (ImPlot::BeginPlot("GPU Timers - Realtime Graph", ImVec2(-1, 250), ImPlotFlags_Crosshairs | ImPlotFlags_NoMouseText))
				{
					// X1 axis - time passed/timeline
					ImPlot::SetupAxis(ImAxis_X1, NULL, ImPlotAxisFlags_Foreground | ImPlotAxisFlags_RangeFit);
					ImPlot::SetupAxisFormat(ImAxis_X1, "%.1f s");
					ImPlot::SetupAxisLimits(ImAxis_X1, g_time_passed - g_graph_history_timespan, g_time_passed, ImGuiCond_Always);

					// Y1 axis - GPU timers
					ImPlot::SetupAxis(ImAxis_Y1, NULL, ImPlotAxisFlags_Foreground | ImPlotAxisFlags_RangeFit);
					ImPlot::SetupAxisFormat(ImAxis_Y1, "%.3f ms");

					// Y2 axis - back buffer index
					ImPlot::SetupAxis(ImAxis_Y2, NULL, ImPlotAxisFlags_Foreground | ImPlotAxisFlags_Invert | ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_RangeFit);
					ImPlot::SetupAxisFormat(ImAxis_Y2, "%.0f");
					ImPlot::SetupAxisLimits(ImAxis_Y2, 0, BACK_BUFFER_COUNT, ImGuiCond_Always);

					// Plot GPU timers
					ImPlot::SetAxes(ImAxis_X1, ImAxis_Y1);
					for (size_t i = 0; i < GPUProfiler::GPUTimer_NumTimers; ++i)
					{
						if (g_scrolling_buffers[i].current_size > 0)
						{
							ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.3f);
							ImPlot::PlotShaded(GetGPUTimerLabel((GPUTimer)i), &g_scrolling_buffers[i].data_buf[0].t, &g_scrolling_buffers[i].data_buf[0].value,
								(int)g_scrolling_buffers[i].current_size, 0.0f, 0, (int)g_scrolling_buffers[i].current_offset, 2 * sizeof(float));

							ImPlot::SetNextLineStyle(IMPLOT_AUTO_COL, 0.3f);
							ImPlot::PlotLine(GetGPUTimerLabel((GPUTimer)i), &g_scrolling_buffers[i].data_buf[0].t, &g_scrolling_buffers[i].data_buf[0].value,
								(int)g_scrolling_buffers[i].current_size, 0, (int)g_scrolling_buffers[i].current_offset, 2 * sizeof(float));
						}
					}

					// Whenever the plot is hovered, show a tooltip which shows the nearest timers (to mouse position in plot)
					if (ImPlot::IsPlotHovered())
					{
						// Get the ImPlot draw list and mouse position in the current plot (gets x and y values of axes of mouse position)
						ImDrawList* draw_list = ImPlot::GetPlotDrawList();
						ImPlotPoint mouse_pos = ImPlot::GetPlotMousePos(ImAxis_X1, ImAxis_Y1);
						ImVec2 left_top_corner = {
							ImPlot::PlotToPixels(mouse_pos.x - 100.0 * 1.5, mouse_pos.y).x,
							ImPlot::PlotToPixels(mouse_pos.x + 100.0 * 1.5, mouse_pos.y).x
						};
						ImVec2 right_bottom_corner = {
							ImPlot::GetPlotPos().y,
							ImPlot::GetPlotPos().y + ImPlot::GetPlotSize().y
						};

						GraphDataNode<float> nearest_nodes[GPUTimer::GPUTimer_NumTimers];

						// Draw mouse hover tooltip
						ImPlot::PushPlotClipRect();
						//draw_list->AddRectFilled(left_top_corner, right_bottom_corner, IM_COL32(128, 128, 128, 64));
						ImGui::BeginTooltip();
						ImGui::Text("Back buffer index: %u", FindNearestDataNode(g_back_buffer_scrolling_buffer, (int)g_back_buffer_scrolling_buffer.current_offset, (int)g_back_buffer_scrolling_buffer.current_size - 1, (float)mouse_pos.x).value);
						ImGui::Text("Frame index: %8llu", FindNearestDataNode(g_frame_index_scrolling_buffer, (int)g_frame_index_scrolling_buffer.current_offset, (int)g_frame_index_scrolling_buffer.current_size - 1, (float)mouse_pos.x).value);

						for (size_t i = 0; i < GPUTimer_NumTimers; ++i)
						{
							// Find nearest data node in the graph from mouse position
							nearest_nodes[i] = FindNearestDataNode(g_scrolling_buffers[(GPUTimer)i], (int)g_scrolling_buffers[(GPUTimer)i].current_offset, (int)g_scrolling_buffers[(GPUTimer)i].current_size - 1, (float)mouse_pos.x);
							ImGui::Text("%s : %.3f ms", GetGPUTimerLabel((GPUTimer)i), nearest_nodes[i].value);
						}
						ImGui::EndTooltip();

						// Draw graph markers for each graph line/GPU timer
						for (size_t i = 0; i < GPUTimer_NumTimers; ++i)
						{
							ImVec2 marker_pos = { ImPlot::PlotToPixels(nearest_nodes[i].t, nearest_nodes[i].value, ImAxis_X1, ImAxis_Y1)};
							draw_list->AddCircleFilled(marker_pos, 10.0f, IM_COL32(192, 192, 192, 192), 20);
						}
						ImPlot::PopPlotClipRect();
					}

					ImPlot::EndPlot();
				}

				ImGui::Unindent(8.0f);
			}

			ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			if (ImGui::CollapsingHeader("Memory Usage"))
			{
				ImGui::Indent(8.0f);

				DXGI_QUERY_VIDEO_MEMORY_INFO info;
				if (SUCCEEDED(g_d3d.dxgi_adapter4->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info)))
				{
					MemoryScope temp;

					ImGui::Text("Current Usage: %s", RT_FormatHumanReadableBytes(temp, info.CurrentUsage));
					ImGui::Text("Total Budget: %s", RT_FormatHumanReadableBytes(temp, info.Budget));
					ImGui::Text("Current Reservation: %s", RT_FormatHumanReadableBytes(temp, info.CurrentReservation));
					ImGui::Text("Available for Reservation: %s", RT_FormatHumanReadableBytes(temp, info.AvailableForReservation));
				}
				else
				{
					ImGui::Text("Failed to query video memory info");
				}

				ImGui::Unindent(8.0f);
			}
		} ImGui::End();
	}

}
