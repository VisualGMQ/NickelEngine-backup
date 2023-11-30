#include "ui/ui.hpp"
#include "core/cgmath.hpp"
#include "ui/event.hpp"

namespace nickel::ui {

void InitSystem(gecs::commands cmds) {
    cmds.emplace_resource<Context>();
}

void doUpdateGlobalPosition(const Style& parentStyle, gecs::entity entity,
                            gecs::registry reg) {
    if (!reg.has<Style>(entity)) {
        return;
    }

    auto& style = reg.get_mut<Style>(entity);
    style.globalCenter_ = parentStyle.globalCenter_ + style.center;

    if (reg.has<Child>(entity)) {
        auto& children = reg.get<Child>(entity);
        for (auto e : children.entities) {
            doUpdateGlobalPosition(style, e, reg);
        }
    }
}

void UpdateGlobalPosition(
    gecs::querier<gecs::mut<Style>, Child, gecs::without<Parent>> root,
    gecs::querier<gecs::mut<Style>, gecs::without<Child, Parent>> q,
    gecs::registry reg) {
    for (auto&& [_, style, child] : root) {
        for (auto c : child.entities) {
            doUpdateGlobalPosition(style, c, reg);
        }
    }

    for (auto&& [_, style] : q) {
        style.globalCenter_ = style.center;
    }
}

void renderStyle(const Style& style, Renderer2D& renderer,
                 const Context& context) {
    auto rect = cgmath::Rect::FromCenter(style.GlobalCenter(),
                                         (style.size + style.padding) * 0.5);
    renderer.FillRect(rect, style.backgroundColor);
    // TODO: draw border with style.borderSize
    rect.position.x -= 1.0;
    rect.position.y -= 1.0;
    rect.size.w += 1.0;
    rect.size.h += 1.0;
    renderer.DrawRect(rect, style.borderColor);
}

void renderButton(gecs::entity ent, const Button& btn,
                  const cgmath::Rect& contentRect, Renderer2D& renderer,
                  const EventRecorder& recorder) {
    const cgmath::Color* color = &btn.color;
    if (recorder.entity == ent) {
        if (recorder.HasEvent(Event::Hover)) {
            color = &btn.hoverColor;
        }
        if (recorder.HasEvent(Event::Press)) {
            color = &btn.pressColor;
        }
    }

    renderer.FillRect(contentRect, *color);
}

void RenderUI(gecs::querier<Style, gecs::without<Parent>> querier,
              gecs::resource<gecs::mut<Renderer2D>> renderer,
              gecs::resource<Context> ctx, gecs::registry reg) {
    renderer->BeginRender(ctx->camera);
    renderer->DisableDepthTest();

    for (auto&& [entity, style] : querier) {
        renderStyle(style, renderer.get(), ctx.get());

        if (reg.has<Button>(entity)) {
            renderButton(entity, reg.get<Button>(entity),
                         cgmath::Rect::FromCenter(style.GlobalCenter(),
                                                  style.size * 0.5),
                         renderer.get(), ctx->eventRecorder);
        }
    }

    renderer->EndRender();
}

inline cgmath::Rect getResponseRect(const Style& s) {
    return cgmath::Rect::FromCenter(s.GlobalCenter(),
                                    (s.size + s.padding) * 0.5);
}

void doDetermineEventEntity(EventRecorder& recorder, const Child& children,
                            const Mouse& mouse, gecs::registry reg) {
    for (auto e : children.entities) {
        if (!reg.has<Style>(e)) continue;

        auto& style = reg.get<Style>(e);
        auto rect = getResponseRect(style);

        if (rect.IsPtIn(mouse.Position())) {
            recorder.entity = e;
            recorder.region = rect;
        }

        if (reg.has<Child>(e)) {
            doDetermineEventEntity(recorder, reg.get<Child>(e), mouse, reg);
        }
    }
}

void HandleEventSystem(gecs::resource<gecs::mut<Context>> ctx,
                       gecs::querier<Style, gecs::without<Parent>> querier,
                       gecs::querier<Style, Child, gecs::without<Parent>> roots,
                       gecs::resource<Mouse> mouse, gecs::registry reg) {
    EventRecorder recorder;

    // check event trigger entity
    for (auto&& [entity, style] : querier) {
        cgmath::Rect rect = getResponseRect(style);
        auto pos = mouse->Position();
        LOGT("mouse position = ", pos);
        if (rect.IsPtIn(pos)) {
            recorder.entity = entity;
            recorder.region = rect;
        }
    }

    for (auto&& [_, style, child] : roots) {
        doDetermineEventEntity(recorder, child, mouse.get(), reg);
    }

    if (recorder.entity == gecs::null_entity) {
        ctx->eventRecorder = std::move(recorder);
        return;
    }

    // check event type
    if (mouse->Offset() != cgmath::Vec2{}) {
        recorder.PushEvent(Event::Hover);
    }

    if (mouse->LeftBtn().IsPress()) {
        recorder.PushEvent(Event::Press);
    }

    if (mouse->LeftBtn().IsRelease()) {
        recorder.PushEvent(Event::Release);
    }

    // deal event
    if (reg.has<EventHandler>(recorder.entity)) {
        auto& handler = reg.get<EventHandler>(recorder.entity);

        if (recorder.HasEvent(Event::Hover)) {
            handler.onHover(recorder.entity, reg);
        }
        if (recorder.HasEvent(Event::Press)) {
            handler.onClick(recorder.entity, reg);
        }
    }

    ctx->eventRecorder = std::move(recorder);
}

}  // namespace nickel::ui